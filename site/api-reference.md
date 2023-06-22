<script setup>
import { data } from './project.data.ts'
const { VERSION } = data;
</script>

# API Reference

A full reference to all available SQL scalar functions, table functions, and virtual tables for `sqlite-vss`.

::: warning
As a reminder, `sqlite-vss` is still young, so breaking changes should be expected while `sqlite-vss` is in a pre-v1 stage!
:::

## `vss0` Virtual Tables

The `vss0` module is used to create virtual tables that store and query your vectors. It takes inspiration from, and is very similar to the [SQLite FTS5 full-text search](https://www.sqlite.org/fts5.html) virtual table.

### Constructor Synax

```sqlite
create virtual table vss_xyz using vss0(
  headline_embedding(384),
  description_embedding(384) factory="IVF4096,Flat,IDMap2"
);
```

The constructor of the `vss0` module takes in a list of column definitions. Currently, each column must be a vector column, where you define the dimensions of the vector as the single argument to the column name. In the above example, both the `headline_embedding` and `description_embedding` columns store vectors with 384 dimensions.

An optional `factory=` option can be placed on individual columns. These are [Faiss factory strings](https://github.com/facebookresearch/faiss/wiki/The-index-factory) that give you more control over how the Faiss index is created. Consult the Faiss documentation to determine which factory makes the most sense for your use case. It's recommended that you include `IDMap2` to your factory string, in order to reconstruct vectors in queries. The default factory string is `"Flat,IDMap2"`, an exhaustive search index.

By contention the table name should be prefixed with `vss_`. If your data exists in a "normal" table named `"xyz"`, then name the vss0 table `vss_xyz`.

### Training

By default, the Faiss indexes storing your vectors do not require any additional training, so you can go straight to inserting data. But if you use a special factory string that requires one, like `"IVF4096,Flat,IDMap2"`, then you'll have to insert training data before using your table. You can do so with the special `operation='training'` constraint.

```sqlite
insert into vss_xyz(operation, description_embedding)
  select 'training', description_embedding from xyz;
```

All training data is read into memory, so take care with large datasets. Not all indexes require the full dataset to train, so you can probably add a `LIMIT N` clause where `N` is an appropriate amount of training vectors. Note that in this example, only the `description_embedding` column needs training, not the `headline_embedding` column that uses the default factory.

### Inserting Data

Data can be insert into `vss0` virtual tables with normal `INSERT INTO` operations.

```sqlite
insert into vss_xyz(rowid, headline_embedding, description_embedding)
  select rowid, headline_embedding, description_embedding from xyz;
```

The vectors themselves can be any JSON or "raw bytes". The rowid is optional, but if your `vss_xyz` table is linked to a `xyz` table, its a good idea to use the same rowids for `JOIN`s later.

#### JSON format

```sqlite
insert into vss_xyz(a_embedding)
  values ('[0.1, 0.2, 0.3]');
```

#### "Raw bytes" format

```sqlite
insert into vss_xyz(a_embedding)
  values (X'cdcccc3dcdcc4c3e9a99993e');
```

#### Transactions

In order for the data to actually insert and appear in the index, make sure to `COMMIT` your inserted data. This is automatically done when using the SQLite CLI, but client libraries like Python will require explicit `.commit()` calls.

### Querying

`vss_xyz` can be queried with `SELECT` statements.

```sqlite
select * from vss_xyz;
```

In order to take advantage of the Faiss indexes for fast KNN (k nearest neighbors), use the `vss_search` function.

::: code-group

```sqlite [SQLite version 3.41+]
select rowid, distance
from vss_xyz
where vss_search(
  headline_embedding,
  (select headline_embedding from xyz where rowid = 123)
)
limit 20
```

```sqlite [SQLite version 3.40 and below]
select rowid, distance
from vss_xyz
where vss_search(
  headline_embedding,
  vss_search_params(
    (select headline_embedding from xyz where rowid = 123),
    20
  )
)
```

:::

Here we get the 20 nearest headline embeddings to the headline_embedding value in row #123. In return we get the rowids of those similar columns, as well as the calculated distance from the query vector.

#### `vss_search()` on SQLite 3.41 and above

Note that `vss_search()` queries with `limit N` only work on SQLite version 3.41 and above, due to a [bug in previous versions](https://sqlite.org/forum/info/6b32f818ba1d97ef).

On lower versions, use [`vss_search_params()`](#vss_search_params) instead:

```sqlite
select rowid, distance
from vss_xyz
where vss_search(
  headline_embedding,
  vss_search_params(
    (select headline_embedding from xyz where rowid = 123),
    20
  )
)
```

This is equivalent to the query above, just a little more verbose.

### Deleting data

`DELETE` operations are supported.

```sqlite
delete from vss_xyz where rowid between 100 and 200;
```

Keep in mind, small `DELETE` operations are ineffiecient, so batch your inserts/deletes and wrap `INSERT`s/`DELETE`s in [transactions](https://www.sqlite.org/lang_transaction.html) whenever possible.

### Shadow Table Schema

You shouldn't need to directly access the shadow tables for `vss0` virtual tables, but here's the format for them. **Subject to change, do not rely on this, will break in the future.**

For a `vss0` virtual table called `xyz`, the follow shadow tables will exist:

- `xyz_data` - One row per "item" in the virtual table. Used to delegate and track rowid usage in the virtual table. `x` is a no-op column. `create table xyz_data(x);`
- `xyz_index` - One row per column index. Stores the raw serialized Faiss index in one big BLOB. `create table xyz_index(idx);`

## `sqlite-vss` Functions

### `vss_version()` {#vss_version}

Returns the version string of the `sqlite-vss` library.

```sqlite-vue
select vss_version();
"{{VERSION}}"
```

### `vss_debug()` {#vss_debug}

Returns a debug string of various info about `sqlite-vss`, including the version string, build date, and commit hash.

```sqlite
select vss_debug();
```

### `vss_search()` {#vss_search}

`vss_search()` is a special function meant to be used in the `WHERE` clause while querying a `vss0` virtual table. It allows you to perform "K nearest neighbors" style searches on a vector table.

```sqlite
create virtual table vss_foo using vss0( bar(4) );

select rowid, distance
from vss_foo
where vss_search(bar, json('[0.1, 0.2, 0.3, 0.4]'))
limit 20;
```

```sqlite
select rowid, distance
from foo
where vss_search(bar, vss_search_params(json(''), 20));
```

### `vss_search_params()` {#vss_search_params}

```sqlite
select vss_search_params(); --
```

### `vss_range_search()` {#vss_range_search}

```sqlite
create virtual table foo using vss0( bar(4) );

select rowid, distance
from foo
where vss_range_search(
  bar,
  vss_range_search_params(json(''), .5)
);
```

### `vss_range_search_params()` {#vss_range_search_params}

```sqlite
select vss_range_search_params(); --
```

### `vss_distance_l1()` {#vss_distance_l1}

Returns the L1 distance between two vectors `a` and `b`. The two arguments must be vectors of the same length. Uses [`fvec_L1()`](https://faiss.ai/cpp_api/file/distances_8h.html#_CPPv4N5faiss7fvec_L1EPKfPKf6size_t)

```sqlite
select vss_distance_l1(
  json('[0, 0]'),
  json('[5, 1]')
);
6.0

select vss_distance_l1(
  json('[2, 3]'),
  json('[5, 1]')
);
5.0
```

### `vss_distance_l2()` {#vss_distance_l2}

Returns the L2 distance between two vectors `a` and `b`. The two arguments must be vectors of the same length. Uses [`fvec_L2sqr()`](https://faiss.ai/cpp_api/file/distances_8h.html#_CPPv4N5faiss10fvec_L2sqrEPKfPKf6size_t)

```sqlite
select vss_distance_l2(
  json('[0, 0]'),
  json('[5, 1]')
);
25.999999999999999999
select vss_distance_l2(
  json('[2, 3]'),
  json('[5, 1]')
);
12.999999999999999999
```

### `vss_distance_linf()` {#vss_distance_linf}

Returns the infinity distance between two vectors `a` and `b`. The two arguments must be vectors of the same length. Uses [`fvec_Linf()`](https://faiss.ai/cpp_api/file/distances_8h.html#_CPPv4N5faiss9fvec_LinfEPKfPKf6size_t)

```sqlite
select vss_distance_linf(
  json('[0, 0]'),
  json('[5, 1]')
);
5.0

select vss_distance_linf(
  json('[2, 3]'),
  json('[5, 1]')
);
3.0
```

### `vss_inner_product()` {#vss_inner_product}

Returns the inner product result between two vectors `a` and `b`. The two arguments must be vectors of the same length. Uses [`fvec_inner_product()`](https://faiss.ai/cpp_api/file/distances_8h.html#_CPPv4N5faiss18fvec_inner_productEPKfPKf6size_t)

```sqlite
select vss_inner_product(
  json('[0, 0]'),
  json('[5, 1]')
);
0.0

select vss_inner_product(
  json('[2, 3]'),
  json('[5, 1]')
);
12.999999999999999999
```

### `vss_fvec_add()` {#vss_fvec_add}

Scalar addition on two vectors `a` and `b`. The two arguments must be vectors of the same length. Uses [`fvec_add()`](https://faiss.ai/cpp_api/file/distances_8h.html#_CPPv4N5faiss8fvec_addE6size_tPKfPKfPf)

```sqlite
select vector_to_json(
  vss_fvec_add(
    json('[0, 0]'),
    json('[1, 2]')
  )
);
'[1.0,2.0]'

select vector_to_json(
  vss_fvec_add(
    json('[2, 5]'),
    json('[-3, 4]')
  )
);
'[-1.0,9.0]'
```

### `vss_fvec_sub()` {#vss_fvec_sub}

Scalar subtraction on two vectors `a` and `b`. The two arguments must be vectors of the same length. Uses [`fvec_sub()`](https://faiss.ai/cpp_api/file/distances_8h.html#_CPPv4N5faiss8fvec_subE6size_tPKfPKfPf)

```sqlite
select vector_to_json(
  vss_fvec_sub(
    json('[0, 0]'),
    json('[1, 2]')
  )
);
'[-1.0,-2.0]'

select vector_to_json(
  vss_fvec_sub(
    json('[2, 5]'),
    json('[-3, 4]')
  )
);
'[5.0,1.0]'
```

## `sqlite-vector` Functions

`sqlite-vector` is much more unstable than `sqlite-vss`, so expect many breaking changes to these functions.

### `vector_version()` {#vector_version}

Returns a version string of the `sqlite-vector` library.

```sqlite
select vector_version();

```

### `vector_debug()` {#vector_debug}

Returns a debug information of the `sqlite-vector` library.

```sqlite
select vector_debug();
```

### `vector0()` {#vector0}

Returns a pointer containing a struct of low-level C API functions and utilities to work with vectors. Uses [SQLite's pointer passing interface](https://www.sqlite.org/bindptr.html). You likely do not need to use this function.

```sqlite
select vector0();
NULL
```

### `vector_length()` {#vector_length}

Returns the length of a vector. TODO fix.

```sqlite
select vector_length(json('[0.1, 0.2, 0.3]'));

```

### `vector_value_at()` {#vector_value_at}

Returns the float value

```sqlite
select vector_value_at();

```

### `vector_from_blob()` {#vector_from_blob}

```sqlite
select vector_from_blob();

```

### `vector_from_json()` {#vector_from_json}

```sqlite
select vector_from_json();

```

### `vector_from_raw()` {#vector_from_raw}

```sqlite
select vector_from_raw();

```

### `vector_to_blob()` {#vector_to_blob}

```sqlite
select vector_to_blob();

```

### `vector_to_json()` {#vector_to_json}

```sqlite
select vector_to_json();

```

### `vector_to_raw()` {#vector_to_raw}

```sqlite
select vector_to_raw();

```
