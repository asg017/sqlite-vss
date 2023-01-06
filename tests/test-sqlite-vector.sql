.load build_release/vss0
.load ../sqlite-vector/build/vector0

select vector_version();
select vss_version();

create virtual table x using vss_index(a(4) with "Flat,IDMap2");

insert into x(rowid, a)
  select 1, vector_from_json('[0.6,0.7,0.8,0.9]');

select vector_debug(vector_from_json('[0.6,0.7,0.8,0.9]'));

.mode box
.header on
select rowid, * from x;