from datasette import hookimpl
import sqlite_vss

@hookimpl
def prepare_connection(conn):
    conn.enable_load_extension(True)
    sqlite_vss.load(conn)
    conn.enable_load_extension(False)