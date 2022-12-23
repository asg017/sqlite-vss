https://github.com/matsui528/faiss_tips

```
cmake -B build; make -C build

cmake -DCMAKE_BUILD_TYPE=Release -B build_release; make -C build_release
```

```bash
cd build/
cmake .. -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF
make
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
create virtual table article_embeddings using faiss_index();

-- Training portion
insert into article_embeddings(operation, vector)
  select 'training', embedding(headline)
  from articles;

insert into article_embeddings(operation) values ('train');


-- adding in data portion
insert into article_embeddings(vector)
  select embedding(headline) from articles;


-- now query it!
select rowid, vector, distance
from article_embeddings(embedding('my query'));
```

```
PYO3_PYTHON=/Users/alex/projects/research-sqlite-vector/venv/bin/python LIBDIR=/usr/local/opt/python@3.8/Frameworks/Python.framework/Versions/3.8/lib cargo build

PYTHONPATH=/Users/alex/projects/research-sqlite-vector/venv/lib/python3.8/site-packages/ sqlite3x :memory: '.read test.sql'
```
