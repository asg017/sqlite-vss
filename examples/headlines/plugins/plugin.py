import json
from datasette import hookimpl

from sentence_transformers import SentenceTransformer
model = SentenceTransformer("sentence-transformers/all-MiniLM-L6-v2")

def st_encode(x):
  result = model.encode([x])
  return json.dumps(result.tolist()[0])

@hookimpl
def prepare_connection(conn):
    conn.create_function('st_encode', 1, st_encode)