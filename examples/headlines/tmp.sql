.load build_release/faiss0

create virtual table temp.article_embeddings using faiss_index(2, "Flat,IDMap");

insert into article_embeddings(operation, vector)
  select 'training', value
  from json_each('[
    [0, 1],
    [0, -1],
    [1, 0],
    [-1, 0]
  ]');

insert into article_embeddings(rowid, vector)
  select key, value
  from json_each('[
    [0, 1],
    [0, -1],
    [1, 0],
    [-1, 0]
  ]');


.mode box
.header on
/*
select rowid, distance from article_embeddings(json('[0.9, 0]'), 5);

select sqlite_version();

select rowid, distance 
from article_embeddings 
where faiss_index_search(vector, json_array(json('[0.9, 0]'), 5))
 and rowid not in (select value from json_each('[1,2,3,4,5]'));
--limit 10
--offset 4;*/


.print search =================================

select rowid, distance 
from article_embeddings 
where faiss_search(article_embeddings.vector, faiss_search_params(json('[0.9, 0]'), 5));

.print range_search =================================
select rowid, distance 
from article_embeddings 
where faiss_range_search(article_embeddings.vector, faiss_range_search_params(json('[0.5, 0.5]'), 1));

.print range_search multiple? =================================
select rowid, distance 
from article_embeddings 
where faiss_range_search(
  article_embeddings.vector, 
  (select faiss_range_search_params(value, 1) from json_each('[[0.5, 0.5], [-0.5, -0.5]]'))
);

.exit

select rowid from article_embeddings where rowid = 2;

delete from article_embeddings where rowid = 2;
/*
insert into article_embeddings(rowid, vector)
  select rowid, py_as_vector(embedding_deserialize(embedding))
  from articles;
*/