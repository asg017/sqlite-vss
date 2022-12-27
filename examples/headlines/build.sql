.bail on
.mode box
.header on

.load build_release/faiss0
.load /Users/alex/projects/sqlite-python/target/debug/libpy0.dylib
.load ../sqlite-lines/dist/lines0
/*
.load build_release/faiss0
drop table article_embeddings; drop table article_embeddings_index;
*/
.timer on

insert into py_define(code) values (readfile('ext-embeddings.py')) returning 'loaded!';

-- 20k: 177s
create table if not exists articles as
select 
  text as headline,
  embedding_serialize(embedding) as embedding
from generate_embeddings((
  select json_group_array(line ->> 'headline') 
  from (
    select line 
    from lines_read('News_Category_Dataset_v3.json')
    limit 2000
  )
));

--create virtual table temp.article_embeddings using templatevtab(384, "IVF50,PQ16np");
create virtual table temp.article_embeddings using templatevtab(384, "IVF10,PQ4");


insert into article_embeddings(operation, vector)
  select 'training', py_as_vector(embedding_deserialize(embedding))
  from articles;

insert into article_embeddings(rowid, vector)
  select rowid, py_as_vector(embedding_deserialize(embedding))
  from articles;



select length(data) from article_embeddings_index;

select 
  article_embeddings.rowid,
  article_embeddings.distance,
  articles.headline
from article_embeddings 
left join articles on articles.rowid = article_embeddings.rowid
where vector = py_as_vector(embedding("racial tensions"));
