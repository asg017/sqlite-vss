FROM datasetteproject/datasette

RUN apt-get update && apt-get install -y curl

WORKDIR /app
RUN curl -L 'https://github.com/asg017/sqlite-vss/releases/download/v0.0.1-alpha.2/sqlite-vss-v0.0.1-alpha.2-ubuntu-x86_64.tar.gz' \
  | tar -xvzf  -
RUN curl -L 'https://github.com/asg017/sqlite-vector/releases/download/v0.0.1-alpha.1/sqlite-vector-v0.0.1-alpha.1-ubuntu-x86_64.tar.gz' \
  | tar -xvzf -

RUN apt-get install -y libgomp1 libatlas-base-dev liblapack-dev 

RUN apt-get install -y cmake git pkg-config build-essential
RUN pip install --upgrade pip
RUN pip install sentence-transformers
RUN python3 -c 'from sentence_transformers import SentenceTransformer; model = SentenceTransformer("sentence-transformers/all-MiniLM-L6-v2");'
ADD plugins/ /app/plugins

CMD datasette /data/headlines.db \
  -p 8001 -h 0.0.0.0 \
  --load-extension=/app/vss0 \
  --load-extension=/app/vector0 \
  --plugins-dir=/app/plugins