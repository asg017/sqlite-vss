# `sqlite-vss` with Datasette

```bash
datasette install datasette-sqlite-vss
```

https://docs.datasette.io/en/stable/plugins.html#deploying-plugins-using-datasette-publish

```bash
datasette publish cloudrun data.db \
  --service=my-service \
  --install=datasette-sqlite-vss
```
