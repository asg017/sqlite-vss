import numpy as np
from io import BytesIO
from sqlite_python_extensions import scalar_function, table_function, Row

from sentence_transformers import SentenceTransformer
model = SentenceTransformer("sentence-transformers/all-MiniLM-L6-v2")

@scalar_function
def embedding(query):
  return np.squeeze(model.encode([query]))

@scalar_function
def embedding_serialize(embedding):
  f = BytesIO()
  np.save(f, embedding)
  f.seek(0)
  return f.read()

@scalar_function
def embedding_deserialize(bytes):
  return np.load(BytesIO(bytes))

import json

@table_function(columns=["text", "embedding"])
def generate_embeddings(input):
  input = json.loads(input)
  print(len(input))
  result = model.encode(input)
  for i, r in enumerate(result):
    yield Row(text=input[i], embedding=r)
  
