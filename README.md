# sqlite-vss

`sqlite-vss` (SQLite <b><u>V</u></b>ector <b><u>S</u></b>imilarity <b><u>S</u></b>earch) is a SQLite extension that brings vector search capabilities to SQLite, based on [Faiss](https://faiss.ai/). It can be used to build semantic search engines, recommendations, or questions-and-answering tools.

See [_Introducing sqlite-vss: A SQLite Extension for Vector Search_](https://observablehq.com/@asg017/introducing-sqlite-vss) (February 2023) for more details and a live example!

If your company or organization finds this library useful, consider [supporting my work](#supporting)!

## Usage

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

`sqlite-vss` is a **Bring-your-own-vectors** database, it is compatable with any embedding or vector data you have. Consider using [OpenAI's Embeddings API](https://platform.openai.com/docs/guides/embeddings), [HuggingFace's Inference API](https://huggingface.co/blog/getting-started-with-embeddings#1-embedding-a-dataset), [`sentence-transformers`](https://www.sbert.net/), or [any of these open source model](https://github.com/embeddings-benchmark/mteb#leaderboard). In this example, we are using [sentence-transformers/all-MiniLM-L6-v2](https://huggingface.co/sentence-transformers/all-MiniLM-L6-v2) to generate embeddings from our text, which have 384 dimensions.

You can insert vectors into `vss0` tables as JSON or raw bytes.

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

You can `INSERT` and `DELETE` into these tables as necessary, but [`UPDATE` operations aren't supported yet](https://github.com/asg017/sqlite-vss/issues/7). This can be used with triggers for automatically updated indexes. Also note that "small" `INSERT`/`DELETE` operations that only insert a few rows can be slow, so batch where necessary.

```sql
begin;

delete from vss_articles
  where rowid between 100 and 200;

insert into vss_articles(rowid, headline_embedding, description_embedding)
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
```

Beware! Indexes that require training can take a long time. With the [News Category Dataset](./examples/headlines/) (386 dimension over 210k vectors) that this example is based on, the default index would take 8 seconds to build. But with the custom `"IVF4096,Flat,IDMap2"` factory, it took 45 minutes to train and 4.5 minutes to insert data! This likely can be reduced with a smaller training set, but the faster queries can be helpful.

## Documentation

See [`docs.md`](./docs.md) for a instructions to compile `sqlite-vss` yourself, as well as a full SQL API reference.

## Installing

The [Releases page](https://github.com/asg017/sqlite-vss/releases) contains pre-built binaries for Linux x86_64 and MacOS x86_64 (MacOS Big Sur 11 or higher). More pre-compiled targets will be available in the future. Additionally, `sqlite-vss` is distributed on common package managers like `pip` for Python and `npm` for Node.js, see below for details.

Do note that on Linux machines, you'll have to install some packages to make these options work:

```
sudo apt-get update
sudo apt-get install -y libgomp1 libatlas-base-dev liblapack-dev
```

> **Note:**
> The `0` in the filename (`vss0.dylib`/ `vss0.so`) denotes the major version of `sqlite-vss`. Currently `sqlite-vss` is pre v1, so expect breaking changes in future versions.

| Language       | Install                                                            | More Info                                                                       |                                                                                                                                                                                           |
| -------------- | ------------------------------------------------------------------ | ------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Python         | `pip install sqlite-vss`                                           | [`sqlite-vss` with Python](https://alexgarcia.xyz/sqlite-vss/python.html)       | [![PyPI](https://img.shields.io/pypi/v/sqlite-vss.svg?color=blue&logo=python&logoColor=white)](https://pypi.org/project/sqlite-vss/)                                                      |
| Datasette      | `datasette install datasette-sqlite-vss`                           | [`sqlite-vss` with Datasette](https://alexgarcia.xyz/sqlite-vss/datasette.html) | [![Datasette](https://img.shields.io/pypi/v/datasette-sqlite-vss.svg?color=B6B6D9&label=Datasette+plugin&logoColor=white&logo=python)](https://datasette.io/plugins/datasette-sqlite-vss) |
| Node.js        | `npm install sqlite-vss`                                           | [`sqlite-vss` with Node.js](https://alexgarcia.xyz/sqlite-vss/nodejs.html)        | [![npm](https://img.shields.io/npm/v/sqlite-vss.svg?color=green&logo=nodedotjs&logoColor=white)](https://www.npmjs.com/package/sqlite-vss)                                                |
| Deno           | [`deno.land/x/sqlite_vss`](https://deno.land/x/sqlite_vss)         | [`sqlite-vss` with Deno](https://alexgarcia.xyz/sqlite-vss/deno.html)           | [![deno version](https://deno.land/badge/sqlite_vss/version?color=fef8d2)](https://deno.land/x/sqlite_vss)                                                                                |
| Ruby           | `gem install sqlite-vss`                                           | [`sqlite-vss` with Ruby](https://alexgarcia.xyz/sqlite-vss/ruby.html)           | ![Gem](https://img.shields.io/gem/v/sqlite-vss?color=red&logo=rubygems&logoColor=white)                                                                                                   |
| Elixir         | [`hex.pm/packages/sqlite_vss`](https://hex.pm/packages/sqlite_vss) | [`sqlite-vss` with Elixir](https://alexgarcia.xyz/sqlite-vss/elixir.html)       | [![Hex.pm](https://img.shields.io/hexpm/v/sqlite_vss?color=purple&logo=elixir)](https://hex.pm/packages/sqlite_vss)                                                                       |
| Go             | `go get -u github.com/asg017/sqlite-vss/bindings/go`               | [`sqlite-vss` with Go](https://alexgarcia.xyz/sqlite-vss/go.html)               | [![Go Reference](https://pkg.go.dev/badge/github.com/asg017/sqlite-vss/bindings/go.svg)](https://pkg.go.dev/github.com/asg017/sqlite-vss/bindings/go)                                     |
| Rust           | `cargo add sqlite-vss`                                             | [`sqlite-vss` with Rust](https://alexgarcia.xyz/sqlite-vss/rust.html)           | [![Crates.io](https://img.shields.io/crates/v/sqlite-vss?logo=rust)](https://crates.io/crates/sqlite-vss)                                                                                 |
| Github Release |                                                                    |                                                                                 | ![GitHub tag (latest SemVer pre-release)](https://img.shields.io/github/v/tag/asg017/sqlite-vss?color=lightgrey&include_prereleases&label=Github+release&logo=github)                     |

### With the `sqlite3` CLI

For using `sqlite-vss` with [the official SQLite command line shell](https://www.sqlite.org/cli.html), download the `vector0.dylib`/`vss0.dylib` (for MacOS Big Sur 11 or higher) or `vector0.so`/`vss0.so` (Linux) files from a release and load it into your SQLite environment.

The `vector0` extension is a required dependency, so make sure to load that before `vss0`.

```sql
.load ./vector0
.load ./vss0
select vss_version();
-- v0.0.1
```

### Python

For Python developers, install the [`sqlite-vss` package](https://pypi.org/package/sqlite-vss/) with:

```
pip install sqlite-vss
```

```python
import sqlite3
import sqlite_vss

db = sqlite3.connect(':memory:')
db.enable_load_extension(True)
sqlite_vss.load(db)

version, = db.execute('select vss_version()').fetchone()
print(version)
```

See [`bindings/python`](./bindings/python/README.md) for more details.

### Node.js

For Node.js developers, install the [`sqlite-vss` npm package](https://www.npmjs.com/package/sqlite-vss) with:

```
npm install sqlite-vss
```

```js
import Database from "better-sqlite3"; // also compatible with node-sqlite3
import * as sqlite_vss from "sqlite-vss";

const db = new Database(":memory:");
sqlite_vss.load(db);

const version = db.prepare("select vss_version()").pluck().get();
console.log(version);
```

See [`npm/sqlite-vss/README.md`](./npm/sqlite-vss/README.md) for more details.

### Deno

For [Deno](https://deno.land/) developers, use the [deno.land/x/sqlite_vss](https://deno.land/x/sqlite_vss) module:

```ts
// Requires all permissions (-A) and the --unstable flag

import { Database } from "https://deno.land/x/sqlite3@0.10.0/mod.ts";
import * as sqlite_vss from "https://deno.land/x/sqlite_vss/mod.ts";

const db = new Database(":memory:");

db.enableLoadExtension = true;
sqlite_vss.load(db);

const [version] = db.prepare("select vss_version()").value<[string]>()!;

console.log(version);
```

See [`deno/sqlite-vss/README.md`](./deno/README.md) for more details.

### Datasette

And for [Datasette](https://datasette.io/), install the [`datasette-sqlite-vss` plugin](https://datasette.io/plugins/datasette-sqlite-vss) with:

```
datasette install datasette-sqlite-vss
```

See [`bindings/datasette`](./bindings/datasette/README.md) for more details.

## Disadvantages

- The underlying Faiss indicies are capped at 1GB. Follow [#1](https://github.com/asg017/sqlite-vss/issues/1) for updates.
- Additional filtering on top of KNN searches aren't supported yet. Follow [#2](https://github.com/asg017/sqlite-vss/issues/2) for updates.
- Only CPU Faiss indicies are supported, not GPU yet. Follow [#3](https://github.com/asg017/sqlite-vss/issues/3) for updates.
- mmap'ed indices aren't supported yet, so indicies have to fit in RAM. Follow [#4](https://github.com/asg017/sqlite-vss/issues/4) for updates.
- This extension is written in C++ and doesn't have fuzzy testing yet. Follow [#5](https://github.com/asg017/sqlite-vss/issues/5) for updates.
- `UPDATE` statements on vss0 virtual tables are not supported, though `INSERT` and `DELETE` statements are. Follow [#7](https://github.com/asg017/sqlite-vss/issues/7) for updates.

## Supporting

I (Alex 👋🏼) spent a lot of time and energy on this project and [many other open source projects](https://github.com/asg017?tab=repositories&q=&type=&language=&sort=stargazers). If your company or organization uses this library (or you're feeling generous), then please consider [sponsoring my work](https://github.com/sponsors/asg017/), sharing this project with a friend, or [hiring me for contract/consulting work](https://alexgarcia.xyz/work.html)!

## See Also

- [`sqlite-http`](https://github.com/asg017/sqlite-http), a SQLite extension for making HTTP requests
- [`sqlite-xsv`](https://github.com/asg017/sqlite-xsv), a fast SQLite extension for querying CSVs
- [`sqlite-loadable-rs`](https://github.com/asg017/sqlite-loadable-rs), a framework for building SQLite extensions in Rust
