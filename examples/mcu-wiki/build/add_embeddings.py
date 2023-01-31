import sqlite3
import sys
import json

db = sqlite3.connect(sys.argv[1])
db.load_extension("./vector0")
db.load_extension("./vss0")


from sentence_transformers import SentenceTransformer
model = SentenceTransformer("sentence-transformers/all-MiniLM-L6-v2")

BATCH_SIZE = 256
total_count = db.execute("select count(*) from plot_sentences where contents_embedding is null").fetchone()[0]

while True:
  batch = db.execute("select rowid, contents from plot_sentences where contents_embedding is null limit ?", [BATCH_SIZE]).fetchall()
  
  if len(batch) == 0:
    break

  rowids = list(map(lambda x: x[0], batch))
  contents = list(map(lambda x: x[1], batch))

  print(f"[{rowids[-1]/total_count*100:.2f}] encoding [{rowids[0]}:{rowids[-1]}]")
  embeddings = model.encode(contents)
  
  for rowid, embedding in zip(rowids, embeddings):
    db.execute("update plot_sentences set contents_embedding = vector_to_blob(vector_from_json(?)) where rowid = ?", [json.dumps(embedding.tolist()), rowid])
  
  db.commit()

