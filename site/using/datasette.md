# `sqlite-vss` with Datasette

[![Datasette](https://img.shields.io/pypi/v/datasette-sqlite-vss.svg?color=B6B6D9&label=Datasette+plugin&logoColor=white&logo=python)](https://datasette.io/plugins/datasette-sqlite-vss)

```bash
datasette install datasette-sqlite-vss
```

https://docs.datasette.io/en/stable/plugins.html#deploying-plugins-using-datasette-publish

```bash
datasette publish cloudrun data.db \
  --service=my-service \
  --install=datasette-sqlite-vss
```
