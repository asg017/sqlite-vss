# `sqlite-vss` with Python

[![PyPI](https://img.shields.io/pypi/v/sqlite-vss.svg?color=blue&logo=python&logoColor=white)](https://pypi.org/project/sqlite-vss/)

Python developers can use `sqlite-vss` with the [`sqlite-vss` PyPi package](https://pypi.org/project/sqlite-vss/).

## Installing

which can be installed with:

```bash
pip install sqlite-vss
```

In Python, the `sqlite_vss` module has a `.load()` function that will load all `sqlite-vss` SQL function into a given SQLite connection.

```python
import sqlite3
import sqlite_vss

db = sqlite3.connect(':memory:')
db.enable_load_extension(True)
sqlite_vss.load(db)

version, = db.execute('select vss_version()').fetchone()
print(version)
```

See [the API Reference](./api-reference) for all available SQL functions.

## Storing Python Vectors

If your vectors in Python are represented as a list of floats, you can insert them into a `vss0` table in a few different ways.

The first approach: serialize your list as a JSON string with [`json.dumps()`](https://docs.python.org/3/library/json.html#json.dumps).

```python
embedding = [0.1, 0.2]
db.execute("insert into vss_demo(a) values (?)", json.dumps(embedding))
```

Alternatively, you can convert them into a compact "raw bytes" format with [`struct.pack()`](https://docs.python.org/3/library/struct.html#struct.pack). This approach will likely use much less storage than the JSON string approach.

```python
import struct

def serialize(vector: List[float]) -> bytes:
  """ serializes a list of floats into a compact "raw bytes" format """
  return struct.pack('%sf' % len(vector), *vector)

embedding = [0.1, 0.2]
db.execute('insert into vss_demo(a) values (?)'', serialize(embedding))
```

If your embeddings are a numpy array, you can use [`.tobytes()`](https://numpy.org/doc/stable/reference/generated/numpy.ndarray.tobytes.html) to convert the array into the `sqlite-vss`-compatible "raw bytes" format.

```python
import numpy as np

embedding = np.array([0.1, 0.2, 0.3])

#db.execute("insert into demo(a) values (?)", embedding.astype(np.float32).tobytes())
db.execute("insert into vss_demo(a) values (?)", embedding.astype(np.float32).tobytes())
```

The `.astype(np.float32)` cast is important! Faiss and `sqlite-vss` works with 4-byte floats, not 8-byte doubles.

## Querying Vectors

```python
import numpy as np

embedding = np.array([0.1, 0.2, 0.3]).astype(np.float32)

for row in db.execute("select vector_to_raw(a) from demo"):

db.execute("insert into vss_demo(a) values (?)", embedding.astype(np.float32).tobytes())
```
