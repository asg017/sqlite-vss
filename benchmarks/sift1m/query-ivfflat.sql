.bail on

.load ./vector0
.load ./vss0
.timer on

select rowid from x_ivfflat limit 1;


-- 1ms
select count(*)
from x_ivfflat
where vss_search(
  v, 
  vss_search_params(
    vector_from_blob((select vector from sift where rowid = 5 limit 1)),
    128
  )
);

-- 26ms
select count(*)
from (select vector from sift limit 128) as x
join x_ivfflat
where vss_search(
  v, 
  vss_search_params(
    vector,
    128
  )
);