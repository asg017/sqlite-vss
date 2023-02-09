
```sql
.param set :id 8217
with similar_matches as (
  select rowid, distance 
  from vss_blog_entry
  where vss_search(
    body_embedding, 
    (select body_embedding from blog_entry where id = :id limit 1)
  )
  limit 10
)
select
  similar_matches.*,
  blog_entry.title,
  blog_entry.body
from similar_matches
left join blog_entry on blog_entry.id = similar_matches.rowid;
```