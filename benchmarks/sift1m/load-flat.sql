.bail on

.load ./vector0
.load ./vss0
.timer on

create virtual table x_flat using vss0(v(128));

insert into x_flat(rowid, v)
  select rowid, vector_from_blob(vector) from sift;

/*select rowid
from temp.x_flat
where vss_search(
  v, 
  vss_search_params(
    vector_from_blob((select vector from sift limit 1)),
    10
  )
);*/