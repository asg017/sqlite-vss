.load build_release/faiss0
.bail on
.header on
.mode box
.timer on


create table t as
  select 
    ( select json_group_array((random() % 100)/ 100.) from generate_series(1, 100)) as vector
    --json_array((random() % 100)/ 100., (random() % 100)/ 100.) as vector
  from generate_series(1, 1e7);

select count(*) from t;

create virtual table temp.article_embeddings using faiss_index(100, "Flat,IDMap");

.print training...
insert into article_embeddings(operation, vector)
  select 'training', vector
  from t;

.print adding...
insert into article_embeddings(rowid, vector)
  select rowid, vector
  from t;

