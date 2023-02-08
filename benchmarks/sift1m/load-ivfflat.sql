.bail on

.load ./vector0
.load ./vss0
.timer on

create virtual table x_ivfflat using vss0(v(128) factory="IVF2048,Flat,IDMap2");

-- 173s
insert into x_ivfflat(operation, rowid, v)
  select 'training', rowid, vector from sift;

insert into x_ivfflat(rowid, v)
  select rowid, vector from sift;

/*select rowid
from x_ivfflat
where vss_search(
  v, 
  vss_search_params(
    vector_from_blob((select vector from sift limit 1)),
    10
  )
);*/