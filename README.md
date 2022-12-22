https://github.com/matsui528/faiss_tips

```bash
cd build/
cmake .. -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF
make

sqlite3x :memory: '.load build/libmyProject' '.mode box' 'select a()' 'select a()' 'select 1'
```

## Prototype

```sql
--select create_index('file.index');
select add_training(data.data) from data;
select train();
select add_data(data.data) from data;
select query(vector);
```

## real deal

```sql
create virtual table article_embeddings
```

```
PYO3_PYTHON=/Users/alex/projects/research-sqlite-vector/venv/bin/python LIBDIR=/usr/local/opt/python@3.8/Frameworks/Python.framework/Versions/3.8/lib cargo build

PYTHONPATH=/Users/alex/projects/research-sqlite-vector/venv/lib/python3.8/site-packages/ sqlite3x :memory: '.read test.sql'
```
