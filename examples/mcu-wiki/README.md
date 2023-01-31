
WIP

1. `sqlite3 test.db '.read build.sql'`
2. `deno run -A --unstable extract_plots.ts test.db`
3. `python3 add_embeddings.py test.db`
4. `sqlite3 test.db '.read add_vss.sql'`


```sql
with matches as (
  select rowid
  from vss_plot_sentences
  where vss_search(contents_embedding, vss_search_params(json(st_encode('who killed thanos?')), 30))
),
final as (
  select 
    plot_sentences.rowid,
    plot_sentences.contents
  from matches
  left join plot_sentences on plot_sentences.rowid = matches.rowid
)
select * from final;
```