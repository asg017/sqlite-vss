#!/bin/bash
sqlite3 test.db '.load ./vector0' '.load ./vss0' \
  'drop table if exists x_flat;' \
  'drop table if exists x_ivfflat;' 