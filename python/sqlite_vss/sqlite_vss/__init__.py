import os
import sqlite3

def vector_loadable_path():
  loadable_path = os.path.join(os.path.dirname(__file__), "vector0")
  return os.path.normpath(loadable_path)

def vss_loadable_path():
  loadable_path = os.path.join(os.path.dirname(__file__), "vss0")
  return os.path.normpath(loadable_path)

def load_vector(conn: sqlite3.Connection)  -> None:
  conn.load_extension(vector_loadable_path())

def load_vss(conn: sqlite3.Connection)  -> None:
  conn.load_extension(vss_loadable_path())

def load(conn: sqlite3.Connection)  -> None:
  load_vector(conn)
  load_vss(conn)
