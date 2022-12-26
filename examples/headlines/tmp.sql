.load build_release/faiss0
.load /Users/alex/projects/sqlite-python/target/debug/libpy0.dylib
.mode box
.echo on

select 1;

--drop table article_embeddings; drop table article_embeddings_index;

create virtual table article_embeddings using templatevtab();

.timer on


insert into py_define(code) values (readfile('ext-embeddings.py')) returning 'loaded!';

insert into article_embeddings(operation, vector)
  select 'training', py_as_vector(embedding_deserialize(embedding))
  from articles;

.print train...
insert into article_embeddings(operation) values ('train');
.print training done.



.print adding...
insert into article_embeddings(rowid, vector)
  select rowid, py_as_vector(embedding_deserialize(embedding))
  from articles;
.print adding done

.print writing...
insert into article_embeddings(operation) values ('debug');
.print writing done.


select 
  article_embeddings.rowid,
  article_embeddings.distance,
  articles.headline
from article_embeddings 
left join articles on articles.rowid = article_embeddings.rowid
where vector = py_as_vector(embedding("sad emotional"));

select writefile("testxx.index", data) from article_embeddings_index limit 1;