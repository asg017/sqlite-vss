## API Reference

<h3 name="vss0"><code>vss0</code> Virtual Tables</h3>

#### Shadow Table Schema

You shouldn't need to directly access the shadow tables for `vss0` virtual tables, but here's the format for them. **Subject to change, do not rely on this, will break in the future.**

- `xyz_data` - One row per "item" in the virtual table. Used to delegate and track rowid usage in the virtual table. `x` is a no-op column. `create table xyz_data(x);`
- `xyz_index` - One row per column index. Stores the raw serialized Faiss index in one big BLOB. `create table xyz_index(c0, c1, ... cN);`
```
create table 
```