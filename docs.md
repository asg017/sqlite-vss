# `sqlite-vss` Documentation

As a reminder, `sqlite-vss` is still young, so breaking changes should be expected while `sqlite-vss` is in a pre-v1 stage.

## Building `sqlite-vss` yourself

If there isn't a prebuilt `sqlite-vss` build for your operating system or CPU architecture, you can try building `sqlite-vss` yourself. It'll require a C++ compiler, cmake, and possibly a few other libraries to build correctly.

Below are the general steps to build `sqlite-vss`. Your operating system may require a few more libraries or setup instructions, so some tips are listed below under [Platform-specific compiling tips](#platform-specific-compiling-tips).

To start, clone this repository and its submodules.

```
git clone --recurse-submodules https://github.com/asg017/sqlite-vss.git
cd sqlite-vss
```

Next, you'll need to build a vendored version of SQLite under `vendor/sqlite`. To retrieve the SQLite amalgammation, run the `./vendor/get_sqlite.sh` script:

```
./vendor/get_sqlite.sh
```

Then navigate to the newly built `vendor/sqlite` directory and build the SQLite library.

```
cd vendor/sqlite
./configure && make
```

Now that all dependencies are downloaded and configured, you can build the `sqlite-vss` extension! Run either `make loadable` or `make loadable-release` to build a loadable SQLite extension.

```
# build a debug version of `sqlite-vss`. Faster to compile, slow at runtime
make loadable

# build a release version of `sqlite-vss`. Slow to compile, but fast at runtime
make loadable-release
```

If you ran `make loadable`, then under `dist/debug` you'll find debug version of `vector0` and `vss0`, with file extensions `.dylib`, `.so`, or `.dll`, depending on your operating system. If you ran `make loadable-release`, you'll find optimized version of `vector0` and `vss`under `dist/release`.

Not all computers support [AVX2 instructions](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions#CPUs_with_AVX2), which can result in a `Illegal instruction (core dumped)` error when trying to `.load ./vss0`. A compile-time option is being [considered](https://github.com/asg017/sqlite-vss/issues/14). As a temporary fix edit the `sqlite-vss/CMakeLists.txt` and replace `faiss_avx2` with `faiss`.

### Platform-specific compiling tips

#### MacOS (x86_64)

On Macs, you may need to install and use `llvm` for compilation. It can be install with brew:

```
brew install llvm
```

Additionally, if you see other cryptic compiling errors, you may need to explicitly state to use the `llvm` compilers, with flags like so:

```
export CC=/usr/local/opt/llvm/bin/clang
export CXX=/usr/local/opt/llvm/bin/clang++
export LDFLAGS="-L/usr/local/opt/llvm/lib"
export CPPFLAGS="-I/usr/local/opt/llvm/include"
```

If you come across any problems, please file an issue!

#### MacOS (M1/M2 arm)

I haven't tried compiling `sqlite-vss` on a M1 Mac yet, but others have reported success. See the above instructions if you have problems, or file an issue.

#### Linux (x86_64)

You most likely will need to install the following libraries before compiling:

```
sudo apt-get update
sudo apt-get install libgomp1 libatlas-base-dev liblapack-dev libsqlite3-dev
```

## API Reference

<h3 name="vss0"><code>vss0</code> Virtual Tables</h3>

The `vss0` module is used to create virtual tables that store and query your vectors.

#### Constructor Synax

```sql
create virtual table vss_xyz using vss0(
  headline_embedding(384),
  description_embedding(384) factory="IVF4096,Flat,IDMap2"
);
```

The constructor of the `vss0` module takes in a list of column definitions. Currently, each column must be a vector column, where you define the dimensions of the vector as the single argument to the column name. In the above example, both the `headline_embedding` and `description_embedding` columns store vectors with 384 dimensions.

An optional `factory=` option can be placed on individual columns. These are [Faiss factory strings](https://github.com/facebookresearch/faiss/wiki/The-index-factory) that give you more control over how the Faiss index is created. Consult the Faiss documentation to determine which factory makes the most sense for your use case. It's recommended that you include `IDMap2` to your factory string, in order to reconstruct vectors in queries. The default factory string is `"Flat,IDMap2"`, an exhaustive search index.

There is also the `metrict_type=` option. These are [Faiss metric types](https://github.com/facebookresearch/faiss/wiki/MetricType-and-distances). The options are `L1`, `L2`, `INNER_PRODUCT`, `Linf`, `Canberra`, `BrayCurtis`, and `JensenShannon`. If you are planning on using the `JensenShannon` metric type, you must L1 normalize inputs as FAISS assumes the inputs are L1 normalized. You should also be aware that FAISS implements this as the JensenShannon divergence, not the JensenShannon distance. 

The `storage_type=` option can be used to specify how you would like indices to be stored. The current options are `faiss_ondisk` and `faiss_shadow`. The on disk option will store the indices on disk in the same directory as your database file. The default option stores indices as blobs in index shadow tables. 

By contention the table name should be prefixed with `vss_`. If your data exists in a "normal" table named `"xyz"`, then name the vss0 table `vss_xyz`.

#### Training

By default, the Faiss indexes storing your vectors do not require any additional training, so you can go straight to inserting data. But if you use a special factory string that requires one, like `"IVF4096,Flat,IDMap2"`, then you'll have to insert training data before using your table. You can do so with the special `operation='training'` constraint.

```sql
insert into vss_xyz(operation, description_embedding)
  select 'training', description_embedding from xyz;
```

All training data is read into memory, so take care with large datasets. Not all indexes require the full dataset to train, so you can probably add a `LIMIT N` clause where `N` is an appropriate amount of training vectors. Note that in this example, only the `description_embedding` column needs training, not the `headline_embedding` column that uses the default factory.

#### Inserting Data

Data can be insert into `vss0` virtual tables with normal `INSERT INTO` operations.

```sql
insert into vss_xyz(rowid, headline_embedding, description_embedding)
  select rowid, headline_embedding, description_embedding from xyz;
```

The vectors themselves can be any JSON or "raw bytes". The rowid is optional, but if your `vss_xyz` table is linked to a `xyz` table, its a good idea to use the same rowids for `JOIN`s later.

In order for the data to actually insert and appear in the index, make sure to `COMMIT` your inserted data. This is automatically done when using the SQLite CLI, but client libraries like Python will require explicit `.commit()` calls.

#### Querying

`vss_xyz` can be queried with `SELECT` statements.

```sql
select * from vss_xyz;
```

In order to take advantage of the Faiss indexes for fast KNN (k nearest neighbors), use the `vss_search` function.

```sql
select rowid, distance
from vss_xyz
where vss_search(
  headline_embedding,
  (select headline_embedding from xyz where rowid = 123)
)
limit 20
```

Here we get the 20 nearest headline embeddings to the headline_embedding value in row #123. In return we get the rowids of those similar columns, as well as the calculated distance from the query vector.

Note that `vss_search()` queries with `limit N` only work on SQLite version 3.41 and above, due to a [bug in previous versions](https://sqlite.org/forum/info/6b32f818ba1d97ef). On lower version, use the `vss_search_params` option:

```sql
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

#### Deleting data

`DELETE` operations are supported.

```sql
delete from vss_xyz where rowid between 100 and 200;
```

#### Shadow Table Schema

You shouldn't need to directly access the shadow tables for `vss0` virtual tables, but here's the format for them. **Subject to change, do not rely on this, will break in the future.**

- `xyz_data` - One row per "item" in the virtual table. Used to delegate and track rowid usage in the virtual table. `x` is a no-op column. `create table xyz_data(x);`
- `xyz_index` - One row per column index. Stores the raw serialized Faiss index in one big BLOB. `create table xyz_index(idx);`

```
create table
```

<h3 name="vss_search"><code>vss_search(vector_column, vector)</code></h3>

```sql
create virtual table foo using vss0( bar(4) );

select rowid, distance
from foo
where vss_search(bar, json(''))
limit 20;
```

```sql
select rowid, distance
from foo
where vss_search(bar, vss_search_params(json(''), 20));
```

<h3 name="vss_search_params"><code>vss_search_params(vector, k)</code></h3>

```sql
select vss_search_params(); --
```

<h3 name="vss_range_search"><code>vss_range_search(vector_column, vector)</code></h3>

```sql
create virtual table foo using vss0( bar(4) );

select rowid, distance
from foo
where vss_range_search(
  bar,
  vss_range_search_params(json(''), .5)
);
```

<h3 name="vss_range_search_params"><code>vss_range_search_params(vector, distance)</code></h3>

```sql
select vss_range_search_params(); --
```

<h3 name="vss_distance_l1"><code>vss_distance_l1(vector1, vector2)</code></h3>

[`fvec_L1()`](https://faiss.ai/cpp_api/file/distances_8h.html#_CPPv4N5faiss7fvec_L1EPKfPKf6size_t)

```sql
select vss_distance_l1(); --
```

<h3 name="vss_distance_l2"><code>vss_distance_l2(vector1, vector2)</code></h3>

[`fvec_L2sqr()`](https://faiss.ai/cpp_api/file/distances_8h.html#_CPPv4N5faiss10fvec_L2sqrEPKfPKf6size_t)

```sql
select vss_distance_l2(); --
```

<h3 name="vss_distance_linf"><code>vss_distance_linf(vector1, vector2)</code></h3>

[`fvec_Linf()`](https://faiss.ai/cpp_api/file/distances_8h.html#_CPPv4N5faiss9fvec_LinfEPKfPKf6size_t)

```sql
select vss_distance_linf(); --
```

<h3 name="vss_inner_product"><code>vss_inner_product(vector1, vector2)</code></h3>

[`fvec_inner_product()`](https://faiss.ai/cpp_api/file/distances_8h.html#_CPPv4N5faiss18fvec_inner_productEPKfPKf6size_t)

```sql
select vss_inner_product(); --
```

<h3 name="vss_fvec_add"><code>vss_fvec_add(vector1, vector2)</code></h3>

[`fvec_add()`](https://faiss.ai/cpp_api/file/distances_8h.html#_CPPv4N5faiss8fvec_addE6size_tPKfPKfPf)

```sql
select vss_fvec_add(); --
```

<h3 name="vss_fvec_sub"><code>vss_fvec_sub(vector1, vector2)</code></h3>

[`fvec_sub()`](https://faiss.ai/cpp_api/file/distances_8h.html#_CPPv4N5faiss8fvec_subE6size_tPKfPKfPf)

```sql
select vss_fvec_sub(); --
```

<h3 name="vss_version"><code>vss_version()</code></h3>

```sql
select vss_version(); --
```

<h3 name="vss_debug"><code>vss_debug()</code></h3>

```sql
select vss_debug(); --
```
