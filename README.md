# sqlite-vss

`sqlite-vss` (SQLite <b><u>V</u></b>ector <b><u>S</u></b>imilarity <b><u>S</u></b>earch) is a SQLite extension that brings vector search capabilities to SQLite, based on [Faiss](https://faiss.ai/) and [`sqlite-vector`](https://github.com/asg017/sqlite-vector). Can be used to build semantic search engines, recommendations, or questions-and-answering tools.

See [_Introducing sqlite-vss: A SQLite Extension for Vector Search_](https://observablehq.com/@asg017/introducing-sqlite-vss) (February 2023) for more details and a live example!

If your company or organization finds this library useful, consider [supporting my work](#supporting)!

## Usage

`sqlite-vss` depends on the [`sqlite-vector`](https://github.com/asg017/sqlite-vector) extension, so load `vector0` before loading `vss0`.

```sql
.load ./vector0
.load ./vss0

select vss_version(); -- 'v0.0.1'

```

`sqlite-vss` has a similar API to the [`fts5` Full-Text Search Extension](https://www.sqlite.org/fts5.html). Use the `vss0` module to create virtual tables that can efficiently store and query your vectors.


```sql
-- 384 == number of dimensions for this example
create virtual table vss_articles using vss0(
  headline_embedding(384),
  description_embedding(384),
);
```

`sqlite-vss` is a **Bring-your-own-vectors** database, it is compatable with any embedding or vectorization data you have. Consider using [OpenAI's Embeddings API](https://platform.openai.com/docs/guides/embeddings), [HuggingFace's Inference API](https://huggingface.co/blog/getting-started-with-embeddings#1-embedding-a-dataset), [`sentence-transformers`](https://www.sbert.net/), or [any of these open source model](https://github.com/embeddings-benchmark/mteb#leaderboard). You can insert vectors into `vss0` tables as JSON, raw bytes, or any format defined in [`sqlite-vector`](https://github.com/asg017/sqlite-vector/blob/main/docs.md).

```sql
insert into vss_articles(rowid, headline_embedding)
  select rowid, headline_embedding from articles;
```

To query for similar vectors ("k nearest neighbors"), use the `vss_search` function in the `WHERE` clause. Here we are searching for the 100 nearest neighbors to the embedding in row #123 in the `articles` table. 

```sql
select rowid, distance
from vss_articles
where vss_search(
  headline_embedding, 
  (select headline_embedding from articles where rowid = 123)
)
limit 100;
```



You can `INSERT` and `DELETE` into these tables as necessary, but do note that [`UPDATE` operations aren't supported yet](https://github.com/asg017/sqlite-vss/issues/7). This can be used with triggers for automatically updated indexes. Also note that "small" `INSERT`/`DELETE` operations that only insert a few rows can be slow, so batch where necessary.

```sql
begin;

delete from vss_articles 
  where rowid between 100 and 200;
insert into vss_articles(rowd, headline_embedding, description_embedding)
  values (:rowid, :headline_embedding, :description_embedding)

commit;
```

You can pass in custom [Faiss factory strings](https://github.com/facebookresearch/faiss/wiki/The-index-factory) for specific columns to control how the Faiss index is stored and queried. By default the factory string is `"Flat,IDMap2"`, which can be slow to query as your database grows. Here, we add an [inverted file index](https://github.com/facebookresearch/faiss/wiki/The-index-factory#inverted-file-indexes) with 4096 centroids, a non-exhaustive option that makes large database queries much faster.

```sql
create virtual table vss_ivf_articles using vss0(
  headline_embedding(384) factory="IVF4096,Flat,IDMap2",
  description_embedding(384) factory="IVF4096,Flat,IDMap2"
);
```

This IVF will require training! You can define training data with a `INSERT` command in a single transaction, with the special `operation="training"` constraint.

```sql
insert into vss_ivf_articles(operation, headline_embedding, description_embedding)
  select 
    'training',
    headline_embedding,
    description_embedding
  from articles;

-- then insert the index data
insert into vss_ivf_articles(rowid, headline_embedding, description_embedding)
  select 
    rowid,
    headline_embedding,
    description_embedding
  from articles;
```

Beware! Indexes that require training can take a long time. With the [News Category Dataset](./examples/headlines/) (386 dimension over 210k vectors) that this example is based on, the default index would take 8 seconds to build. But with the custom `"IVF4096,Flat,IDMap2"` factory, it took 45 minutes to train and 4.5 minutes to insert data! This likely can be reduced with a smaller training set, but the faster queries can be helpful.

## Documentation

See [`docs.md`](./docs.md) for a full API reference.

## Installing

The [Releases page](https://github.com/asg017/sqlite-vss/releases) contains pre-built binaries for Linux x86_64 and MacOS x86_64. More pre-compiled options will be available in the future.

Do note that on Linux machines, you'll have to install some packages to make it work:

```
sudo apt-get update
sudo apt-get install -y libgomp1 libatlas-base-dev liblapack-dev 
```

### As a loadable extension

If you want to use `sqlite-vss` as a [Runtime-loadable extension](https://www.sqlite.org/loadext.html), Download the `vss0.dylib` (for MacOS) or `vss0.so` (Linux) file from a release and load it into your SQLite environment.

`sqlite-vector` is a required dependency, so also make sure to [install that loadable extension](https://github.com/asg017/sqlite-vector#Installing) before loading `vss0`.

> **Note:**
> The `0` in the filename (`vss0.dylib`/ `vss0.so`/`vss0.dll`) denotes the major version of `sqlite-vss`. Currently `sqlite-vss` is pre v1, so expect breaking changes in future versions.

For example, if you are using the [SQLite CLI](https://www.sqlite.org/cli.html), you can load the library like so:

```sql
.load ./vector0
.load ./vss0
select vss_version();
-- v0.0.1
```

Or in Python, using the builtin [sqlite3 module](https://docs.python.org/3/library/sqlite3.html):

```python
import sqlite3
con = sqlite3.connect(":memory:")
con.enable_load_extension(True)
con.load_extension("./vector0")
con.load_extension("./vss0")
print(con.execute("select vss_version()").fetchone())
# ('v0.1.0',)
```

Or in Node.js using [better-sqlite3](https://github.com/WiseLibs/better-sqlite3):

```javascript
const Database = require("better-sqlite3");
const db = new Database(":memory:");
db.loadExtension("./vector0");
db.loadExtension("./vss0");
console.log(db.prepare("select vss_version()").get());
// { 'vss_version()': 'v0.1.0' }
```

Or with [Datasette](https://datasette.io/):

```
datasette data.db --load-extension ./vector0 --load-extension ./vss0
```

## Disadvantages

- The underlying Faiss indicies are capped at 1GB. Follow [#1](https://github.com/asg017/sqlite-vss/issues/1) for updates.
- Additional filtering on top of KNN searches aren't supported yet. Follow [#2](https://github.com/asg017/sqlite-vss/issues/2) for updates.
- Only CPU Faiss indicies are supported, not GPU yet. Follow [#3](https://github.com/asg017/sqlite-vss/issues/3) for updates.
- mmap'ed indices aren't supported yet, so indicies have to fit in RAM. Follow [#4](https://github.com/asg017/sqlite-vss/issues/4) for updates.
- This extension is written in C++ and doesn't have fuzzy testing yet. Follow [#5](https://github.com/asg017/sqlite-vss/issues/5) for updates.
- `UPDATE` statements on vss0 virtual tables are not supported, though `INSERT` and `DELETE` statements are. Follow [#7](https://github.com/asg017/sqlite-vss/issues/7) for updates.


## Supporting

I (Alex 👋🏼) spent a lot of time and energy on this project and [many other open source projects](https://github.com/asg017?tab=repositories&q=&type=&language=&sort=stargazers). If your company or organization uses this library (or you're feeling generous), then please [consider supporting my work](https://alexgarcia.xyz/work.html), or share this project with a friend!

## See Also

- [`sqlite-http`](https://github.com/asg017/sqlite-http), a SQLite extension for making HTTP requests
- [`sqlite-xsv`](https://github.com/asg017/sqlite-xsv), a fast SQLite extension for querying CSVs
- [`sqlite-loadable-rs`](https://github.com/asg017/sqlite-loadable-rs), a framework for building SQLite extensions in Rust
