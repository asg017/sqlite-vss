.bail on
--.mode box
.header on

.load build/vss0

select vss_version();

create virtual table x using vss_index( a(2) with "Flat,IDMap2", b(1) with "Flat,IDMap2");

select * from x_data;

insert into x(rowid, a, b)
  select 
    key + 1000, 
    json_extract(value, '$[0]'),
    json_extract(value, '$[1]')
  from json_each('[
    [[0, 1], [1]], 
    [[0, -1], [2]], 
    [[1, 0], [3]], 
    [[-1, 0], [4]]
  ]');


select * from x where vss_search(a, vss_search_params(json('[0, 0.9]'), 2));

select * from x where vss_search(a, vss_search_params(json('[0, 0.9]'), 3));