FROM datasetteproject/datasette

RUN apt-get update && apt-get install -y curl


RUN apt-get install -y libgomp1 libatlas-base-dev liblapack-dev 

RUN apt-get install -y cmake git pkg-config build-essential
RUN pip install --upgrade pip
RUN pip install sentence-transformers
RUN python3 -c 'from sentence_transformers import SentenceTransformer; model = SentenceTransformer("sentence-transformers/all-MiniLM-L6-v2");'
ADD plugins/ /app/plugins

WORKDIR /sqlite
RUN curl -L 'https://www.sqlite.org/snapshot/sqlite-snapshot-202302081247.tar.gz'  \
  |tar -xvzf -
WORKDIR /sqlite/sqlite-snapshot-202302081247
RUN ./configure
RUN make
ENV LD_LIBRARY_PATH=/sqlite/sqlite-snapshot-202302081247/.libs


WORKDIR /app
RUN curl -L 'https://github.com/asg017/sqlite-vss/releases/download/v0.0.1-alpha.7/sqlite-vss-v0.0.1-alpha.7-ubuntu-x86_64.tar.gz' \
  | tar -xvzf  -
RUN curl -L 'https://github.com/asg017/sqlite-vector/releases/download/v0.0.1-alpha.5/sqlite-vector-v0.0.1-alpha.5-ubuntu-x86_64.tar.gz' \
  | tar -xvzf -


CMD datasette /data/headlines.db \
  -p 8001 -h 0.0.0.0 --cors \
  --load-extension=/app/vector0 \
  --load-extension=/app/vss0 \
  --plugins-dir=/app/plugins
