# `sqlite-vss` with Python

[![PyPI](https://img.shields.io/pypi/v/sqlite-vss.svg?color=blue&logo=python&logoColor=white)](https://pypi.org/project/sqlite-vss/)

Python developers can use `sqlite-vss` with the [`sqlite-vss` PyPi package](https://pypi.org/project/sqlite-vss/).

```bash
pip install sqlite-vss
```

In Python, the `sqlite_vss` module has a `.load()` function that will load `sqlite-vss` SQL functions into a given SQLite connection.

```python
import sqlite3
import sqlite_vss

db = sqlite3.connect(':memory:')
db.enable_load_extension(True)
sqlite_vss.load(db)
db.enable_load_extension(False)

version, = db.execute('select vss_version()').fetchone()
print(version)
```

Checkout [the API Reference](./api-reference) for all available SQL functions.

Also see _[Making SQLite extensions pip install-able](https://observablehq.com/@asg017/making-sqlite-extensions-pip-install-able) (February 2023)_ for more information on how pip install'able SQLite extensions work.

## Working with Vectors in Python

### Vectors as JSON

If your vectors in Python are represented as a list of floats, you can insert them into a `vss0` table as a JSON string with [`json.dumps()`](https://docs.python.org/3/library/json.html#json.dumps).

```python
import json

embedding = [0.1, 0.2, 0.3]
db.execute("insert into vss_demo(a) values (?)", [json.dumps(embedding)])
```

### Vectors as Bytes

You can also convert a list of floats into the compact "raw bytes" format with [`struct.pack()`](https://docs.python.org/3/library/struct.html#struct.pack).

```python
import struct

def serialize(vector: List[float]) -> bytes:
  """ serializes a list of floats into a compact "raw bytes" format """
  return struct.pack('%sf' % len(vector), *vector)

embedding = [0.1, 0.2, 0.3]
db.execute('insert into vss_demo(a) values (?)', [serialize(embedding)])
```

### `numpy` Arrays

If your embeddings are a numpy array, you can use [`.tobytes()`](https://numpy.org/doc/stable/reference/generated/numpy.ndarray.tobytes.html) to convert the array into the `sqlite-vss`-compatible "raw bytes" format.

```python
import numpy as np

embedding = np.array([0.1, 0.2, 0.3])

db.execute(
    "insert into vss_demo(a) values (?)", [embedding.astype(np.float32).tobytes()]
)
```

The `.astype(np.float32)` cast is important! Faiss and `sqlite-vss` works with 4-byte floats, not 8-byte doubles.

### Which format should I choose?

If you're inserting into a `vss0` table, the vector will be converting into a compact Faiss format anyway, so inserting a vector as JSON or "raw bytes" has no effect on the storage size of your `vss0` virtual table.

However, if you are also saving the vector in a regular SQLite column, then the "raw bytes" format will be much more compact than the JSON string representation.

You may also find the "raw bytes" method to be slightly faster than `json.dumps()`, especially in `numpy`.
