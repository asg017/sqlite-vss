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
    #
    #            |
    #    1000 -> X 
    #            |          1002
    #            |          /
    #            |         V
    # ---X-----------------X--
    #    ^1001   |
    #            |
    #            |
    #            X <- 1003
    #            |
    #
    #
    execute_all('create virtual table x using vss_index(2, "Flat,IDMap2");')
    execute_all("""
      insert into x(rowid, vector)
        select key + 1000, value
        from json_each(?);
      """, ['[[0, 1], [0, -1], [1, 0], [-1, 0]]'])
    db.commit()

    self.assertEqual(execute_all("select count(*) from x_index")[0]["count(*)"], 1)
    self.assertEqual(execute_all("select rowid from x_data"), [
      {"rowid": 1000},
      {"rowid": 1001},
      {"rowid": 1002},
      {"rowid": 1003},
    ])

    def search_x(v, k):
      return execute_all("select rowid, distance from x where vss_search(vector, vss_search_params(json(?), ?))", [v, k])
    
    def range_search_x(v, d):
      return execute_all("select rowid, distance from x where vss_range_search(vector, vss_range_search_params(json(?), ?))", [v, d])
    
    self.assertEqual(search_x('[0.9, 0]', 5), [
      {'rowid': 1002, 'distance': 0.010000004433095455},
      {'rowid': 1000, 'distance': 1.809999942779541},
      {'rowid': 1001, 'distance': 1.809999942779541},
      {'rowid': 1003, 'distance': 3.609999895095825}
    ])
    self.assertEqual(range_search_x('[0.5, 0.5]', 1), [
      {'rowid': 1000, 'distance': 0.5},
      {'rowid': 1002, 'distance': 0.5},
    ])
    self.assertEqual(execute_all('select rowid, vector from x'), [
      {'rowid': 1000, "vector": "[0.0,1.0]"},
      {'rowid': 1001, "vector": "[0.0,-1.0]"},
      {'rowid': 1002, "vector": "[1.0,0.0]"},
      {'rowid': 1003, "vector": "[-1.0,0.0]"},
    ])

    self.assertEqual(
      explain_query_plan("select * from x where vss_search(vector, null);"),
      "SCAN x VIRTUAL TABLE INDEX 1:search"
    )
    self.assertEqual(
      explain_query_plan("select * from x where vss_range_search(vector, null);"),
      "SCAN x VIRTUAL TABLE INDEX 2:range_search"
    )
    self.assertEqual(
      explain_query_plan("select * from x"),
      "SCAN x VIRTUAL TABLE INDEX 3:fullscan"
    )

    # TODO support rowid point queries

    self.assertEqual(execute_all("select name from pragma_table_list where name like 'x%' order by 1"),[
      {"name": "x"},
      {"name": "x_data"},
      {"name": "x_index"},
    ])
    execute_all("drop table x;")
    self.assertEqual(execute_all("select name from pragma_table_list where name like 'x%' order by 1"),[])
    

class TestCoverage(unittest.TestCase):                                      
  def test_coverage(self):                                                      
    test_methods = [method for method in dir(TestVss) if method.startswith('test_vss')]
    funcs_with_tests = set([x.replace("test_", "") for x in test_methods])
    for func in FUNCTIONS:
      self.assertTrue(func in funcs_with_tests, f"{func} does not have cooresponding test in {funcs_with_tests}")

if __name__ == '__main__':
    unittest.main()