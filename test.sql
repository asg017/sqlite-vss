.bail on
.mode box
.header on

.load build/libmyProject
.load /Users/alex/projects/sqlite-python/target/debug/libpy0.dylib

insert into py_define(code)
  values ('
import numpy as np
from sqlite_python_extensions import scalar_function, table_function, Row

@scalar_function
def x():
  return np.array([1, 2, 3, 4])


from sentence_transformers import SentenceTransformer
model = SentenceTransformer("sentence-transformers/all-MiniLM-L6-v2")

@scalar_function
def embedding(query):
  return np.squeeze(model.encode([query]))

');



select py_as_vector(x());
select faiss_vector_debug(py_as_vector(x()));
select faiss_vector_debug(py_as_vector(embedding("my name is alex garcia")));