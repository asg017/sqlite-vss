#!/bin/bash

hyperfine --cleanup=./cleanup.sh \
  'sqlite3 test.db ".read load-flat.sql"' \
  'sqlite3 test.db ".read load-ivfflat.sql"'