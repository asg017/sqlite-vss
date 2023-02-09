#!/bin/bash
LD_LIBRARY_PATH=/root/tmp/py/sqlite-snapshot-202302081247/.libs datasette test.db --host 0.0.0.0 \
  --load-extension=../../../sqlite-vector/build_release/vector0 \
  --load-extension=../../build_release/vss0
