.bail on

.load ./vector0
.load ./vss0
.timer on

create virtual table x_ivfflat using vss0(v(128) factory="IVF4096,Flat,IDMap2");

-- 2048: 146s, 4096: 550s
insert into x_ivfflat(operation, rowid, v)
  select 'training', rowid, vector from sift;

-- 2048: 42s, 4096: 68s
insert into x_ivfflat(rowid, v)
  select rowid, vector from sift;