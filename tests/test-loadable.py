import sqlite3
import unittest
import time
import os

EXT_PATH="./build_release/vss0"


def connect(ext):
  db = sqlite3.connect(":memory:")

  db.execute("create table base_functions as select name from pragma_function_list")
  db.execute("create table base_modules as select name from pragma_module_list")

  db.enable_load_extension(True)
  db.load_extension(ext)

  db.execute("create temp table loaded_functions as select name from pragma_function_list where name not in (select name from base_functions) order by name")
  db.execute("create temp table loaded_modules as select name from pragma_module_list where name not in (select name from base_modules) order by name")

  db.row_factory = sqlite3.Row
  return db


db = connect(EXT_PATH)

def explain_query_plan(sql):
  return db.execute("explain query plan " + sql).fetchone()["detail"]

def execute_all(sql, args=None):
  if args is None: args = []
  results = db.execute(sql, args).fetchall()
  return list(map(lambda x: dict(x), results))

FUNCTIONS = [
  'vss_range_search',
  'vss_range_search_params',
  'vss_search',
  'vss_search_params',
]

MODULES = [
  "vss_index",
]
class TestVss(unittest.TestCase):
  def test_funcs(self):
    funcs = list(map(lambda a: a[0], db.execute("select name from loaded_functions").fetchall()))
    self.assertEqual(funcs, FUNCTIONS)

  def test_modules(self):
    modules = list(map(lambda a: a[0], db.execute("select name from loaded_modules").fetchall()))
    self.assertEqual(modules, MODULES)

    
  #def test_lines_version(self):
  #  with open("./VERSION") as f:
  #    version = f.read()
  #  
  #  self.assertEqual(db.execute("select lines_version()").fetchone()[0], version)
#
  #def test_lines_debug(self):
  #  debug = db.execute("select lines_debug()").fetchone()[0].split('\n')
  #  self.assertEqual(len(debug), 3)

  def test_vss_search(self):
    self.skipTest("TODO")

  def test_vss_search_params(self):
    self.skipTest("TODO")

  def test_vss_range_search(self):
    self.skipTest("TODO")

  def test_vss_range_search_params(self):
    self.skipTest("TODO")

    
  def test_vss_index(self):
    execute_all('create virtual table x using vss_index(2, "Flat,IDMap");')
    execute_all("""
      insert into x(rowid, vector)
        select key, value
        from json_each(?);
      """, ['[[0, 1], [0, -1], [1, 0], [-1, 0]]'])

    def search_x(v, k):
      return execute_all("select rowid, distance from x where vss_search(vector, vss_search_params(json(?), ?))", [v, k])
    
    def range_search_x(v, d):
      return execute_all("select rowid, distance from x where vss_range_search(vector, vss_range_search_params(json(?), ?))", [v, d])
    
    self.assertEqual(search_x('[0.9, 0]', 5), [
      {'distance': 0.010000004433095455, 'rowid': 2},
      {'distance': 1.809999942779541, 'rowid': 0},
      {'distance': 1.809999942779541, 'rowid': 1},
      {'distance': 3.609999895095825, 'rowid': 3}
    ])
    self.assertEqual(range_search_x('[0.5, 0.5]', 1), [
      {'distance': 0.5, 'rowid': 0},
      {'distance': 0.5, 'rowid': 2},
    ])
    self.assertEqual(
      explain_query_plan("select * from x where vss_range_search(vector, null);"),
      "SCAN x VIRTUAL TABLE INDEX 0:range_search"
    )
    self.assertEqual(
      explain_query_plan("select * from x where vss_search(vector, null);"),
      "SCAN x VIRTUAL TABLE INDEX 0:search"
    )
    # TODO support rowid query plans
    #self.assertEqual(
    #  explain_query_plan("select * from x"),
    #  "SCAN x VIRTUAL TABLE INDEX 0:rowid"
    #)

class TestCoverage(unittest.TestCase):                                      
  def test_coverage(self):                                                      
    test_methods = [method for method in dir(TestVss) if method.startswith('test_vss')]
    funcs_with_tests = set([x.replace("test_", "") for x in test_methods])
    for func in FUNCTIONS:
      self.assertTrue(func in funcs_with_tests, f"{func} does not have cooresponding test in {funcs_with_tests}")

if __name__ == '__main__':
    unittest.main()