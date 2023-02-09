#!/bin/bash
set -euo pipefail
DB=$1

sqlite3 $DB '.read build/import_articles.sql'
python3 build/add_embeddings.py $DB
sqlite3 $DB '.read build/add_vector_search.sql'
