.bail on
.mode box
.header on

.load build_release/faiss0
.load /Users/alex/projects/sqlite-python/target/debug/libpy0.dylib
.load ../sqlite-lines/dist/lines0

.timer on


insert into py_define(code) values (readfile('ext-embeddings.py')) returning 'loaded!';


insert into templatevtab(operation, vector)
  select 'training', py_as_vector(embedding_deserialize(embedding))
  from articles;

.print train...
insert into templatevtab(operation) values ('train');
.print training done.


.print adding...
insert into templatevtab(rowid, vector)
  select rowid, py_as_vector(embedding_deserialize(embedding))
  from articles;
.print adding done

.print writing...
insert into templatevtab(operation) values ('debug');
.print writing done.


.print querying...
select 
  templatevtab.rowid,
  templatevtab.distance,
  articles.headline
from templatevtab 
left join articles on articles.rowid = templatevtab.rowid
where vector = py_as_vector(embedding("science fiction"));


select 
  templatevtab.rowid,
  templatevtab.distance,
  articles.headline
from templatevtab 
left join articles on articles.rowid = templatevtab.rowid
where vector = py_as_vector(embedding("sad emotional"));


select 
  templatevtab.rowid,
  templatevtab.distance,
  articles.headline
from templatevtab 
left join articles on articles.rowid = templatevtab.rowid
where vector = py_as_vector(embedding("racial tensions"));



select 
  templatevtab.rowid,
  templatevtab.distance,
  articles.headline
from templatevtab 
left join articles on articles.rowid = templatevtab.rowid
where vector = py_as_vector(embedding("social media"));