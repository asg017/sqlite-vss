.bail on
.load ./vector0
.load ./vss0
.timer on
.header on
.mode box


create virtual table vss_siftsmall using vss_index(vector(128) with "Flat,IDMap2");

insert into vss_siftsmall(rowid, vector)
  select 
    rowid, vector
  from vector_fvecs_each(readfile('data/siftsmall/siftsmall_base.fvecs'));

select 
  rowid,
  distance
from vss_siftsmall
where vss_search(
  vector, 
  vss_search_params((select vector from vector_fvecs_each(readfile('data/siftsmall/siftsmall_query.fvecs')) limit 1), 5)
);



.exit
select 
  rowid, dimensions, vector_to_json(vector)
from vector_fvecs_each(readfile('data/siftsmall/siftsmall_base.fvecs'))
limit 10;

select  count(*) from vector_fvecs_each(readfile('data/siftsmall/siftsmall_base.fvecs'));
select  count(*) from vector_fvecs_each(readfile('data/sift/sift_base.fvecs'));
--select  count(*) from vector_fvecs_each(readfile('data/sift/sift_base.fvecs'));