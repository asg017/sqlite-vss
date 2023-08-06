from sqlite_utils import hookimpl
import sqlite_vss

from sqlite_utils_sqlite_vss.version import __version_info__, __version__


@hookimpl
def prepare_connection(conn):
    conn.enable_load_extension(True)
    sqlite_vss.load(conn)
    conn.enable_load_extension(False)
