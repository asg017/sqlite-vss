.bail on

.load ./vector0
.load ./vss0
.timer on

create virtual table x_flat using vss0(v(128));

-- 10s
insert into x_flat(rowid, v)
  select rowid, vector from sift;