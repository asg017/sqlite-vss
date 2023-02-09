# The `datasette-sqlite-vss` Datasette Plugin

`datasette-sqlite-vss` is a [Datasette plugin](https://docs.datasette.io/en/stable/plugins.html) that loads the [`sqlite-vss`](https://github.com/asg017/sqlite-vss) extension in Datasette instances.
```
datasette install datasette-sqlite-vss
```

See [`docs.md`](../../docs.md) for a full API reference for the TODO SQL functions.

Alternatively, when publishing Datasette instances, you can use the `--install` option to install the plugin.

```
datasette publish cloudrun data.db --service=my-service --install=datasette-sqlite-vss

```
