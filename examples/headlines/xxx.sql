curl https://api.openai.com/v1/embeddings \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $OPENAI_API_KEY" \
  -d '{
    "input": ["dogs are great", "health care is a human right"],
    "model": "text-embedding-ada-002"
  }'

with chunk as (
  select
    rowid,
    headline,
    row_number() over (order by rowid) - 1 as idx
  from articles
  where articles.headline_embedding is null
  order by 1
  limit 1024
),
chunk_with_embeddings as (
  select
    rowid,
    headline_embeddings.embedding as headline_embedding
  from chunk
  left join openai_embeddings('text-embedding-ada-002') as headline_embeddings
    on headline_embeddings.index = chunk.idx
  where headline_embeddings.input in (select headline from chunk)
)
update articles
set articles.headline_embedding = chunk.headline_embedding
from chunk
where chunk.rowid = articles.rowid;
