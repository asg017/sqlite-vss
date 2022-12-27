.bail on
.mode box
.header on

.load build_release/faiss0
.load /Users/alex/projects/sqlite-python/target/debug/libpy0.dylib

.timer on

insert into py_define(code) values (readfile('ext-embeddings.py')) returning 'loaded!';


select 
  article_embeddings.rowid,
  article_embeddings.distance,
  articles.headline
from article_embeddings 
left join articles on articles.rowid = article_embeddings.rowid
where vector = py_as_vector(embedding("science fiction"));


select 
  article_embeddings.rowid,
  article_embeddings.distance,
  articles.headline
from article_embeddings 
left join articles on articles.rowid = article_embeddings.rowid
where vector = py_as_vector(embedding("sad emotional"));


select 
  article_embeddings.rowid,
  article_embeddings.distance,
  articles.headline
from article_embeddings 
left join articles on articles.rowid = article_embeddings.rowid
where vector = py_as_vector(embedding("racial tensions"));



select 
  article_embeddings.rowid,
  article_embeddings.distance,
  articles.headline
from article_embeddings 
left join articles on articles.rowid = article_embeddings.rowid
where vector = py_as_vector(embedding("social media"));