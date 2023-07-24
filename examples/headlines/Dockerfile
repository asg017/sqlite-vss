FROM datasetteproject/datasette

RUN apt-get update && apt-get install -y curl


RUN apt-get install -y libgomp1 libatlas-base-dev liblapack-dev 

RUN apt-get install -y cmake git pkg-config build-essential
RUN pip install --upgrade pip
RUN pip install sentence-transformers
RUN python3 -c 'from sentence_transformers import SentenceTransformer; model = SentenceTransformer("sentence-transformers/all-MiniLM-L6-v2");'
ADD plugins/ /app/plugins

# SQLite v3.41.2 
WORKDIR /sqlite
ADD https://www.sqlite.org/2023/sqlite-autoconf-3410200.tar.gz .
RUN tar -xvzf sqlite-autoconf-3410200.tar.gz
WORKDIR /sqlite/sqlite-autoconf-3410200
RUN ./configure
RUN make
ENV LD_LIBRARY_PATH=/sqlite/sqlite-autoconf-3410200/.libs

#  TODO outdated, use the datasette-sqlite-vss plugin instead

RUN datasette install datasette-sqlite-vss

CMD datasette /data/headlines.db \
  -p 8001 -h 0.0.0.0 --cors \
  --plugins-dir=/app/plugins
