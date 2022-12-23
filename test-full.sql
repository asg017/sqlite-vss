.bail on
.mode box
.header on

--.load build/faiss0
.load build_release/faiss0
.load /Users/alex/projects/sqlite-python/target/debug/libpy0.dylib
.load ../sqlite-lines/dist/lines0

.timer on

/*select vector(1,2,4);
select vector_print(vector(1,2,4));
select length(vector_to_blob(vector(1, 2))), hex(vector_to_blob(vector(1, 2)));
select length(vector_to_blob(vector(1))), hex(vector_to_blob(vector(1)));
select length(vector_to_blob(vector())), hex(vector_to_blob(vector()));
.exit*/

insert into py_define(code) values (readfile('ext-embeddings.py')) returning 'loaded!';

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

.exit
create table if not exists articles as
  select 
    line ->> 'headline' as headline,
    embedding_serialize(embedding(line ->> 'headline')) as embedding
  from lines_read('News_Category_Dataset_v3.json')
  ;


.exit
select id, py_as_vector(result)
  from embeddings((
    select 
      json_group_array(json_array(rowid, headline)) 
      from (
        select 
          rowid, 
          headline 
        from articles 
        limit 1000
      )
    )
  );

.exit
.exit

insert into templatevtab(operation, vector)
  select 'training', py_as_vector(result)
  from embeddings((select json_group_array(headline) from (select headline from articles limit 1000)));


.print train...

insert into templatevtab(operation) values ('train');
.print training done.

.print adding...
insert into templatevtab(rowid, vector)
  select rowid, py_as_vector(embedding(headline))
  from articles
  limit 1000;
.print adding done

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

/*
insert into templatevtab(rowid, vector)
  select key, value from json_each('[1,2,3]');

.print ~~~middle~~~

insert into templatevtab(rowid, vector)
  select key, value from json_each('[4, 5]');


.print ~~~begin~~~
begin;

insert into templatevtab(rowid, vector)
  select key, value from json_each('[4, 5]');

insert into templatevtab(rowid, vector)
  select key, value from json_each('[4, 5]');
.print ~~~committing~~~
commit;*/


.exit


.timer on



select count(add_training(py_as_vector(result)))
from embeddings((select json_group_array(headline) from (select headline from articles limit 600)));

select train();

.exit

create table t2 as
select
  
    add_training(py_as_vector(embedding(headline))) as result --count(py_as_vector(embedding(headline)))
  
from articles
limit 100;

select count(result) from t;

--select train();
--select add_training(py_as_vector(embedding("Low-tech common sense about filenames. The holy trinity is:")));