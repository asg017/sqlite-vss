import unittest
import sqlite3
import sqlite_vss

class TestSqliteVectorPython(unittest.TestCase):
  def test_path(self):
    self.assertEqual(type(sqlite_vss.vss_loadable_path()), str)
    self.assertEqual(type(sqlite_vss.vector_loadable_path()), str)
  
  def test_loads(self):
    db = sqlite3.connect(':memory:')
    db.enable_load_extension(True)
    sqlite_vss.load(db)

    version, = db.execute('select vector_version()').fetchone()
    self.assertEqual(version[0], "v")

    version, = db.execute('select vss_version()').fetchone()
    self.assertEqual(version[0], "v")

    # test individual load functions

    db2 = sqlite3.connect(':memory:')
    db2.enable_load_extension(True)

    sqlite_vss.load_vector(db2)
    version, = db.execute('select vector_version()').fetchone()
    self.assertEqual(version[0], "v")
    
    with self.assertRaisesRegex(sqlite3.OperationalError, "no such function: vss_version"):
      db2.execute('select vss_version()').fetchone()

    sqlite_vss.load_vss(db2)
    version, = db.execute('select vss_version()').fetchone()
    self.assertEqual(version[0], "v")
    
if __name__ == '__main__':
    unittest.main()