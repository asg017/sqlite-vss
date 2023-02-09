
.load ./vector0

create table sift as
  select 
    rowid, vector_to_blob(vector) as vector
  from vector_fvecs_each(readfile('../../examples/sift/data/sift/sift_base.fvecs'));
  

--select rowid, vector_length(vector_from_blob(vector)) from sift;
