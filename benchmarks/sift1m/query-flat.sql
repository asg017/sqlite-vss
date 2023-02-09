.bail on

.load ./vector0
.load ./vss0
.timer on

select rowid from x_flat limit 1;

-- point, k=128 on nq=1 80ms
select count(*)
from x_flat
where vss_search(
  v, 
  vss_search_params(
    (select vector from sift where rowid = 5 limit 1),
    128
  )
);


-- many, k=128 on nq=128, 11s!
select count(*)
from (select vector from sift limit 128) as x
join x_flat
where vss_search(
  v, 
  vss_search_params(
    vector,
    128
  )
);