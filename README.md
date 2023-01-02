## TODO

- [ ] `faiss_` -> `vss_`
- [ ] new `%w_data` table with only rowid's
- [ ] constructor, `dimensions=128, factory="asdfasdf", colA, colB`
- [ ] multiple vector columns
- [ ] INSERT respect transactions
  - `pTable->vectors_to_add`
  - `xCommit`/`xRollback`
- [ ] DELETE and UPDATE support
  - [ ] xUpdate DELETE, with `pTable->ids_to_data` and `xCommit`/`xRollback` support
  - [ ] update, call remove_id, add_id?
- [ ] get reconstructed vector?
- [ ] clustering?
- [ ] [Distances](https://faiss.ai/cpp_api/file/distances_8h.html)
- [ ] [extra distances](https://faiss.ai/cpp_api/file/extra__distances_8h.html)

```sql
create table articles(
  headline text,
  body text
);
create virtual table article_vectors using vss(
  headline(384) using "Flat,IDMap",
  body(384) using "IVF10,PQ4",
);

with similar_headlines as (
  select
    id,
    distance
  from article_vectors
  where vss_search(headline, vss_params('query', :query, 'k', 50));
)
select
  articles.headline
from similar_headlines
left join articles on articles.rowid = similar_headlines.id

```

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

```
PYO3_PYTHON=/Users/alex/projects/research-sqlite-vector/venv/bin/python LIBDIR=/usr/local/opt/python@3.8/Frameworks/Python.framework/Versions/3.8/lib cargo build

PYTHONPATH=/Users/alex/projects/research-sqlite-vector/venv/lib/python3.8/site-packages/ sqlite3x :memory: '.read test.sql'
```
