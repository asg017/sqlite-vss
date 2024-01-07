### 1. Prepare scripts

1. Download lines0 library from [sqlite-lines extension](https://github.com/asg017/sqlite-lines/releases) to current folder

2. Create a Deno script to proxy SQL queries with extensions loaded query.ts

```
import { Database } from "https://deno.land/x/sqlite3/mod.ts";
import * as sqlite_vss from "https://deno.land/x/sqlite_vss/mod.ts";

const db = new Database("headlines.db");

db.enableLoadExtension = true;
sqlite_vss.load(db);

db.loadExtension('./lines0');

const sqlQuery = Deno.args[0];

const stmt = db.prepare(sqlQuery);

let result;
if (Deno.args.length > 1) {
  result = stmt.all(...Deno.args.slice(1));
} else {
   result = stmt.all(); 
}

for (const row of result) {
  console.log(row);
}
```

3. Copy [add_embeddings.py](https://github.com/asg017/sqlite-vss/blob/main/examples/headlines/build/add_embeddings.py)

4. Create generate_embedding.py

```
import json
import sys
from sentence_transformers import SentenceTransformer

model = SentenceTransformer("sentence-transformers/all-MiniLM-L6-v2")

def st_encode(x):
    result = model.encode([x])
    return json.dumps(result.tolist()[0])

x = " ".join(sys.argv[1:])

encoded_result = st_encode(x)

print(encoded_result)
```

### 2. Create database

1. Create articles table

```
deno run --unstable -A query.ts "
create table articles(
  headline text,
  headline_embedding fvector,
  description text,
  description_embedding fvector,
  link text,
  category text,
  authors text,
  date
);
```

2. Import dataset into articles table

```
deno run --unstable -A query.ts "
insert into articles(headline, description, link, category, authors, date)
  select
    line ->> '$.headline'           as headline,
    line ->> '$.short_description'  as description,
    line ->> '$.link'               as link,
    line ->> '$.category'           as category,
    line ->> '$.authors'            as authors,
    line ->> '$.date'               as date
  from lines_read('News_Category_Dataset_v3.json');
"
```

3. Locally generate vector embeddings for each of articles (can take a while)

```
python3 add_embeddings.py headlines.db
```

4. Create vss_articles virtual table

```
deno run --unstable -A query.ts "
create virtual table vss_articles using vss0(
  headline_embedding(384),
  description_embedding(384),
);
"
```

5. Copy embeddings for all rows from articles to vss_articles

```
insert into vss_articles(rowid, headline_embedding, description_embedding)
  select 
    rowid,
    headline_embedding,
    description_embedding
  from articles;
```

### 3. Perform similarity Search

1. Given specific row id

```
row_id=123

deno run --unstable -A /Users/gur/Documents/jan5/Jan05-2039.ts "
select
  rowid,
  distance
from vss_articles
where vss_search(
  headline_embedding,
  (
    select headline_embedding
    from articles
    where rowid = $row_id
  )
)
limit 20;
"
```

2. Given arbitrary search string

```
search_term="first"

deno run --unstable -A query.ts "
with similar_matches as (
  select rowid
  from vss_articles
  where vss_search(
    headline_embedding,
    vss_search_params(?, 5)
  )
), final as (
  select
    articles.headline, articles.link
  from similar_matches
  left join articles on articles.rowid = similar_matches.rowid
)
select * from final;
" "$(python3 generate_embedding.py $search_term)"
```
