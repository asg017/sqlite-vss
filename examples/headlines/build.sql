.bail on
.mode box
.header on

.load build_release/faiss0
.load /Users/alex/projects/sqlite-python/target/debug/libpy0.dylib
.load ../sqlite-lines/dist/lines0

.timer on

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
    limit 20000
  )
));

-- 20k: 177s