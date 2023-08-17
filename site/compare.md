# Comparisons with...

`sqlite-vss`

In general, `sqlite-vss` has the following unique features:

- **Embeddable**: No separate process, server, or configuration is needed, `sqlite-vss` runs in the same process as your application, just like SQLite.
- **Pure SQL**: There is no separate DSL or special API for inserting or querying vectors, it's all SQL.
- **Configurable**: You can specify different [Faiss factory strings](https://github.com/facebookresearch/faiss/wiki/The-index-factory) to customize how your vectors are indexed.
- **Easy Distribution and Installation**: `sqlite-vss` can be installed through several different package managers and languages, including Python, Node.js, Ruby, Deno, Go, rust, and more.

But it also has the following disadvantages:

- **Doesn't scale to millions of users**: A SQLite database can get you very far, but will eventually hit a limit with enough users or with extremely write-heavy applications.
- **Depends on Faiss**: Faiss makes it tricky to compile yourself and hard to use on certain platforms.
- **Young in development**: [`sqlite-vss`'s disadvantages listed on the README](https://github.com/asg017/sqlite-vss#disadvantages) go over several implementation-specific problems you may face using `sqlite-vss`, mostly because `sqlite-vss` is young and has 1 main developer. Consider [sponsoring my work](https://github.com/sponsors/asg017) to have these problems fixed!

This page goes over other vector databases and vector storage techniques that exist, and how `sqlite-vss` differs.

## Vector Databases (Pinecone, Milvus, Qdrant, etc.) { #vector-dbs }

In general, "real" vector databases like [Pinecone](https://www.pinecone.io/), [Milvus](https://milvus.io/), [Qdrant](https://qdrant.tech/), and dozens of others will likely "scale" better than `sqlite-vss`. These databases perform writes faster and more efficiently than `sqlite-vss`, can handle a large number of users at the same time, and can perform metadata-filtering on queries.

On the other hand, `sqlite-vss` doesn't require an entire new server to maintain, is open source, and is most likely "fast enough" for many use-cases.


## JSON or Pickle

## Faiss

## `datasette-faiss`

## Chroma

## pgvector

## txtai
