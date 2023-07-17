# `sqlite-vss` with Datasette

[![Datasette](https://img.shields.io/pypi/v/datasette-sqlite-vss.svg?color=B6B6D9&label=Datasette+plugin&logoColor=white&logo=python)](https://datasette.io/plugins/datasette-sqlite-vss)

`sqlite-vss` is also distributed as a [Datasette plugin](https://docs.datasette.io/en/stable/plugins.html) with [`datasette-sqlite-vss`](https://pypi.org/project/datasette-sqlite-vss/), which can be installed like so:

```bash
datasette install datasette-sqlite-vss
```

`datasette-sqlite-vss` is just a PyPi package and can also be installed with `pip`. However, `datasette install` ensures that the plugin will be downloaded in the same Python environment as your Datasette installation, and will automatically include `sqlite-vss` in future Datasette instances.

If you're using the [`datasette publish`](https://docs.datasette.io/en/stable/publish.html) command, you can [use the `--install` flag](https://docs.datasette.io/en/stable/plugins.html#deploying-plugins-using-datasette-publish) to include `datasette-sqlite-vss` in your Datasette projects.

```bash
datasette publish cloudrun data.db \
  --service=my-service \
  --install=datasette-sqlite-vss
```
