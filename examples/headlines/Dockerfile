FROM datasetteproject/datasette


ADD https://xxxx /app/vss0.so
ADD https://xxxx /app/vector0.so

CMD datasette /mnt/vss.db \
  -p 8001 -h 0.0.0.0 \
  --load-extension=/app/vss0 \
  --load-extension=/app/vector0