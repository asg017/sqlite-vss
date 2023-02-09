create table gist as
  select 
    rowid, vector_to_blob(vector) as vector
  from vector_fvecs_each(readfile('../../examples/sift/data/gist/gist_base.fvecs'));
  