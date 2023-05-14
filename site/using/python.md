# `sqlite-vss` with Python

```python
import struct
import json

embedding = json.loads('[0.1, 0.2, 0.3, 0.7, 0.8, 0.9]')
buffer = struct.pack('%sf' % len(embedding), *embedding)

print(embedding)
print(buffer)
```

1. `struck` pack and unpack
2. numpy
