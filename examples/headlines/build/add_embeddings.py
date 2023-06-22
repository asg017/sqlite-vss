import sqlite3
import sys

print(sys.argv)
db_path = sys.argv[1]

db = sqlite3.connect(db_path)

db.enable_load_extension(True)
db.load_extension("./vector0")
db.enable_load_extension(False)


from sentence_transformers import SentenceTransformer
model = SentenceTransformer("sentence-transformers/all-MiniLM-L6-v2")

BATCH_SIZE = 256
total_count = db.execute("select count(*) from articles where headline_embedding is null").fetchone()[0]

# by default, the headline_embedding and description_embeddings are NULL. This script will fill them in
# with the embeddings from the sentence-transformers model, based on the headline and description columns.


def fill_headline_embeddings():
  # headline_embedding is set to NULL by default. Loop through all rows and fill that column
  # until they are all non-null.
  while True:

    # batch BATCH_SIZE rows at a time. Makes model.encode faster and uses less memory than all-at-once
    batch = db.execute(
      """
        select
          rowid,
          headline
        from articles
        where headline_embedding is null
        limit ?
      """,
      [BATCH_SIZE]
    ).fetchall()

    if len(batch) == 0:
      break

    rowids = list(map(lambda x: x[0], batch))
    headlines = list(map(lambda x: x[1], batch))

    print(f"[{rowids[-1]/total_count*100:.2f}] encoding [{rowids[0]}:{rowids[-1]}]")
    embeddings = model.encode(headlines)

    for rowid, embedding in zip(rowids, embeddings):
      db.execute(
        """
          update articles
          set headline_embedding = ?
          where rowid = ?
        """,
        [embedding.tobytes(), rowid]
      )

    db.commit()

def fill_description_embedding():
  # description_embedding is set to NULL by default. Loop through all rows and fill that column
  # until they are all non-null.
  while True:
    batch = db.execute(
      """
        select
          rowid,
          description
        from articles
        where description_embedding is null
        limit ?
      """,
      [BATCH_SIZE]
    ).fetchall()

    if len(batch) == 0:
      break

    rowids = list(map(lambda x: x[0], batch))
    descriptions = list(map(lambda x: x[1], batch))

    print(f"[{rowids[-1]/total_count*100:.2f}] encoding [{rowids[0]}:{rowids[-1]}]")
    embeddings = model.encode(descriptions)

    for rowid, embedding in zip(rowids, embeddings):
      db.execute(
        """
          update articles
          set description_embedding = ?
          where rowid = ?
        """,
        [embedding.tobytes(), rowid]
      )

    db.commit()

fill_headline_embeddings()
fill_description_embedding()
