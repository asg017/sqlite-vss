
1. `sqlite3 full.db '.read import_articles.sql'`
2. `python3 add_embeddings.py full.db`
3. `sqlite3 full.db '.read add_vector_search.sql'`

```sql
with similar_matches as (
  select rowid
  from vss_articles
  where vss_search(
    headline_embedding, 
    vss_search_params(json(st_encode('charlie brown')), 20)
  )
), final as (
  select 
    articles.*
  from similar_matches
  left join articles on articles.rowid = similar_matches.rowid
)
select * from final;
```

```
datasette full.db --host 0.0.0.0 --load-extension=../../build/vss0.so --load-extension=./vector0.so --plugins-dir=./plugins
```

```
docker build -t x .

docker run -p 8001:8001 -v $PWD/test.db:/mnt/test.db --rm -it  x

docker run -p 8001:8001 --rm -it \
  -v $PWD/build.py:/build.py \
  -v $PWD/test.db:/test.db \
  -v $PWD/News_Category_Dataset_v3.json:/News_Category_Dataset_v3.json \
  x python3 /build.py /test.db /News_Category_Dataset_v3.json 
```



datasette test.db  -p 8001 -h 0.0.0.0 \
  --plugins-dir=plugins \
  --load-extension ./vss0 \
  --load-extension ./vector0


```

```