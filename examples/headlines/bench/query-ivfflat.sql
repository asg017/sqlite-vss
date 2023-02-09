.load ../../../sqlite-vector/build_release/vector0
.load ../../build_release/vss0
.timer on

select rowid from vss_articles limit 1;

with similar_matches as (
  select 
    articles.rowid as base,
    vss_ivf_articles.rowid
  from (select rowid, headline_embedding from articles limit 128) as articles
  join vss_ivf_articles
  where vss_search(
    vss_ivf_articles.headline_embedding, 
    vss_search_params(articles.headline_embedding, 128)
  )
)
select count(*) from similar_matches;