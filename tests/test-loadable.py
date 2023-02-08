import sqlite3
import unittest
import time
import os

EXT_PATH="./build/vss0"
EXT_VECTOR_PATH="../sqlite-vector/build/vector0"


def connect(path=":memory:"):
  db = sqlite3.connect(path)

  
  db.enable_load_extension(True)
  db.load_extension(EXT_VECTOR_PATH)

  db.execute("create temp table base_functions as select name from pragma_function_list")
  db.execute("create temp table base_modules as select name from pragma_module_list")

  db.load_extension(EXT_PATH)

  db.execute("create temp table loaded_functions as select name from pragma_function_list where name not in (select name from base_functions) order by name")
  db.execute("create temp table loaded_modules as select name from pragma_module_list where name not in (select name from base_modules) order by name")

  db.row_factory = sqlite3.Row
  return db


db = connect()

def explain_query_plan(sql):
  return db.execute("explain query plan " + sql).fetchone()["detail"]

def execute_all(cursor, sql, args=None):
  if args is None: args = []
  results = cursor.execute(sql, args).fetchall()
  return list(map(lambda x: dict(x), results))

FUNCTIONS = [
  'vss_debug',
  'vss_distance_l1',
  'vss_distance_l2',
  'vss_distance_linf',
  'vss_fvec_add',
  'vss_fvec_sub',
  'vss_inner_product',
  'vss_range_search',
  'vss_range_search_params',
  'vss_search',
  'vss_search_params',
  'vss_version',
]

MODULES = [
  "vss0",
]
class TestVss(unittest.TestCase):
  def test_funcs(self):
    funcs = list(map(lambda a: a[0], db.execute("select name from loaded_functions").fetchall()))
    self.assertEqual(funcs, FUNCTIONS)

  def test_modules(self):
    modules = list(map(lambda a: a[0], db.execute("select name from loaded_modules").fetchall()))
    self.assertEqual(modules, MODULES)

    
  def test_vss_version(self):
    self.assertEqual(db.execute("select vss_version()").fetchone()[0][0], "v")

  def test_vss_debug(self):
    debug = db.execute("select vss_debug()").fetchone()[0].split('\n')
    self.assertEqual(len(debug), 3)
  
  def test_vss_distance_l1(self):
    vss_distance_l1 = lambda a, b: db.execute("select vss_distance_l1(json(?), json(?))", [a, b]).fetchone()[0]
    self.assertEqual(vss_distance_l1('[0, 0]', '[0, 0]'), 0.0)
    self.assertEqual(vss_distance_l1('[0, 0]', '[0, 1]'), 1.0)

  def test_vss_distance_l2(self):
    vss_distance_l2 = lambda a, b: db.execute("select vss_distance_l2(json(?), json(?))", [a, b]).fetchone()[0]
    self.assertEqual(vss_distance_l2('[0, 0]', '[0, 0]'), 0.0)
    self.assertEqual(vss_distance_l2('[0, 0]', '[0, 1]'), 1.0)

  def test_vss_distance_linf(self):
    vss_distance_linf = lambda a, b: db.execute("select vss_distance_linf(json(?), json(?))", [a, b]).fetchone()[0]
    self.assertEqual(vss_distance_linf('[0, 0]', '[0, 0]'), 0.0)
    self.assertEqual(vss_distance_linf('[0, 0]', '[0, 1]'), 1.0)
  
  def test_vss_inner_product(self):
    vss_inner_product = lambda a, b: db.execute("select vss_inner_product(json(?), json(?))", [a, b]).fetchone()[0]
    self.assertEqual(vss_inner_product('[0, 0]', '[0, 0]'), 0.0)
    self.assertEqual(vss_inner_product('[0, 0]', '[0, 1]'), 0.0)
  
  def test_vss_fvec_add(self):
    vss_fvec_add = lambda a, b: db.execute("select vss_fvec_add(json(?), json(?))", [a, b]).fetchone()[0]
    self.assertEqual(vss_fvec_add('[0, 0]', '[0, 0]'), None)
    self.skipTest("TODO")
  
  def test_vss_fvec_sub(self):
    vss_fvec_sub = lambda a, b: db.execute("select vss_fvec_sub(json(?), json(?))", [a, b]).fetchone()[0]
    self.assertEqual(vss_fvec_sub('[0, 0]', '[0, 0]'), None)
    self.skipTest("TODO")
    
  def test_vss_search(self):
    self.skipTest("TODO")

  def test_vss_search_params(self):
    self.skipTest("TODO")

  def test_vss_range_search(self):
    self.skipTest("TODO")

  def test_vss_range_search_params(self):
    self.skipTest("TODO")

    
  def test_vss0(self):
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
    cur = db.cursor()
    execute_all(cur, """
      create virtual table x using vss0( a(2) factory="Flat,IDMap2", b(1)  factory="Flat,IDMap2");
    """)
    execute_all(cur, """
      insert into x(rowid, a, b)
        select 
          key + 1000, 
          json_extract(value, '$[0]'),
          json_extract(value, '$[1]')
        from json_each(?);
      """, ["""
        [
          [[0, 1], [1]], 
          [[0, -1], [2]], 
          [[1, 0], [3]], 
          [[-1, 0], [4]],
          [[0, 0], [5]]
        ]
        """])
    db.commit()
    
    self.assertEqual(cur.lastrowid, 1004)
    self.assertEqual(execute_all(cur, "select rowid, length(idx) from x_index"), [
      {'length(idx)': 170, 'rowid': 0}, 
      {'length(idx)': 150, 'rowid': 1}
    ])
    self.assertEqual(execute_all(cur, "select rowid from x_data"), [
      {"rowid": 1000},
      {"rowid": 1001},
      {"rowid": 1002},
      {"rowid": 1003},
      {"rowid": 1004},
    ])
    execute_all(cur, "delete from x where rowid = 1004")
    db.commit()

    self.assertEqual(execute_all(cur, "select rowid from x_data"), [
      {"rowid": 1000},
      {"rowid": 1001},
      {"rowid": 1002},
      {"rowid": 1003},
    ])

    self.assertEqual(execute_all(cur, "select rowid, length(idx) from x_index"), [
      {'length(idx)': 154, 'rowid': 0}, 
      {'length(idx)': 138, 'rowid': 1}
    ])

    def search(column, v, k):
      return execute_all(cur, f"select rowid, distance from x where vss_search({column}, vss_search_params(json(?), ?))", [v, k])
    
    def range_search(column, v, d):
      return execute_all(cur, f"select rowid, distance from x where vss_range_search({column}, vss_range_search_params(json(?), ?))", [v, d])
    
    self.assertEqual(search('a', '[0.9, 0]', 5), [
      {'rowid': 1002, 'distance': 0.010000004433095455},
      {'rowid': 1000, 'distance': 1.809999942779541},
      {'rowid': 1001, 'distance': 1.809999942779541},
      {'rowid': 1003, 'distance': 3.609999895095825}
    ])
    self.assertEqual(search('b', '[6]', 2), [
      {'rowid': 1003, 'distance': 4.0},
      {'rowid': 1002, 'distance': 9.0},
    ])
    
    with self.assertRaisesRegex(sqlite3.OperationalError, 'input query size doesn\'t match index dimensions: 0 != 1'):
      search('b', '[]', 2)

    with self.assertRaisesRegex(sqlite3.OperationalError, 'input query size doesn\'t match index dimensions: 3 != 1'):
      search('b', '[0.1, 0.2, 0.3]', 2)
    
    with self.assertRaisesRegex(sqlite3.OperationalError, 'k must be greater than 0, got -1'):
      search('b', '[6]', -1)
    
    with self.assertRaisesRegex(sqlite3.OperationalError, 'k must be greater than 0, got 0'):
      search('b', '[6]', 0)

    self.assertEqual(range_search('a', '[0.5, 0.5]', 1), [
      {'rowid': 1000, 'distance': 0.5},
      {'rowid': 1002, 'distance': 0.5},
    ])
    self.assertEqual(range_search('b', '[2.5]', 1), [
      {'rowid': 1001, 'distance': 0.25},
      {'rowid': 1002, 'distance': 0.25},
    ])

    #import pdb;pdb.set_trace()

    self.assertEqual(execute_all(cur, 'select rowid, vector_debug(a) as a, vector_debug(b) as b, distance from x'), [
      {'rowid': 1000, "a": "size: 2 [0.000000, 1.000000]",  "b": "size: 1 [1.000000]", "distance": None},
      {'rowid': 1001, "a": "size: 2 [0.000000, -1.000000]", "b": "size: 1 [2.000000]", "distance": None},
      {'rowid': 1002, "a": "size: 2 [1.000000, 0.000000]",  "b": "size: 1 [3.000000]", "distance": None},
      {'rowid': 1003, "a": "size: 2 [-1.000000, 0.000000]", "b": "size: 1 [4.000000]", "distance": None},
    ])

    if sqlite3.sqlite_version_info[1] >= 41:
      self.assertEqual(
        execute_all(
          cur, 
          f"select rowid, distance from x where vss_search(a, json(?)) limit ?", 
          ['[0.9, 0]', 2]
        ),
        [
          {'distance': 0.010000004433095455, 'rowid': 1002},
          {'distance': 1.809999942779541, 'rowid': 1000}
        ]
      )
      with self.assertRaisesRegex(sqlite3.OperationalError, "2nd argument to vss_search\(\) must be a vector"):
        execute_all(
            cur, 
            f"select rowid, distance from x where vss_search(a, 3) limit 1"
          )
    
    else:
      with self.assertRaisesRegex(sqlite3.OperationalError, "vss_search\(\) only support vss_search_params\(\) as a 2nd parameter for SQLite versions below 3.41.0"):
        execute_all(
            cur, 
            f"select rowid, distance from x where vss_search(a, json(?)) limit ?", 
            ['[0.9, 0]', 2]
          )
    
    self.assertRegex(
      explain_query_plan("select * from x where vss_search(a, null);"),
      r'SCAN (TABLE )?x VIRTUAL TABLE INDEX 0:search'
    )
    self.assertRegex(
      explain_query_plan("select * from x where vss_search(b, null);"),
      r'SCAN (TABLE )?x VIRTUAL TABLE INDEX 1:search'
    )
    self.assertRegex(
      explain_query_plan("select * from x where vss_range_search(a, null);"),
      r'SCAN (TABLE )?x VIRTUAL TABLE INDEX 0:range_search'
    )
    self.assertRegex(
      explain_query_plan("select * from x where vss_range_search(b, null);"),
      r'SCAN (TABLE )?x VIRTUAL TABLE INDEX 1:range_search'
    )
    self.assertRegex(
      explain_query_plan("select * from x"),
      r'SCAN (TABLE )?x VIRTUAL TABLE INDEX -1:fullscan'
    )

    # TODO support rowid point queries

    self.assertEqual(db.execute("select count(*) from x_data").fetchone()[0], 4)
    
    execute_all(cur, "drop table x;")
    with self.assertRaisesRegex(sqlite3.OperationalError, "no such table: x_data"):
      self.assertEqual(db.execute("select count(*) from x_data").fetchone()[0], 4)
    
    with self.assertRaisesRegex(sqlite3.OperationalError, "no such table: x_index"):
      self.assertEqual(db.execute("select count(*) from x_index").fetchone()[0], 2)
    
    
  def test_vss0_persistent(self):
    import tempfile
    tf = tempfile.NamedTemporaryFile(delete=False)
    tf.close()
    
    db = connect(tf.name)
    db.execute("create table t as select 1 as a")
    cur = db.cursor()
    execute_all(cur, """
      create virtual table x using vss0( a(2), b(1) factory="Flat,IDMap2");
    """)
    execute_all(cur, """
      insert into x(rowid, a, b)
        select 
          key + 1000, 
          json_extract(value, '$[0]'),
          json_extract(value, '$[1]')
        from json_each(?);
      """, ["""
        [
          [[0, 1], [1]], 
          [[0, -1], [2]], 
          [[1, 0], [3]], 
          [[-1, 0], [4]]
        ]
        """])

    db.commit()

    def search(cur,  column, v, k):
      return execute_all(cur, f"select rowid, distance from x where vss_search({column}, vss_search_params(json(?), ?))", [v, k])
    
    self.assertEqual(search(cur, 'a', '[0.9, 0]', 5), [
      {'rowid': 1002, 'distance': 0.010000004433095455},
      {'rowid': 1000, 'distance': 1.809999942779541},
      {'rowid': 1001, 'distance': 1.809999942779541},
      {'rowid': 1003, 'distance': 3.609999895095825}
    ])
    self.assertEqual(search(cur, 'b', '[6]', 2), [
      {'rowid': 1003, 'distance': 4.0},
      {'rowid': 1002, 'distance': 9.0},
    ])
    db.close()

    db = connect(tf.name)
    cur = db.cursor()
    self.assertEqual(execute_all(db.cursor(), "select a from t"), [{"a": 1}])
    self.assertEqual(search(cur, 'a', '[0.9, 0]', 5), [
      {'rowid': 1002, 'distance': 0.010000004433095455},
      {'rowid': 1000, 'distance': 1.809999942779541},
      {'rowid': 1001, 'distance': 1.809999942779541},
      {'rowid': 1003, 'distance': 3.609999895095825}
    ])
    self.assertEqual(search(cur, 'b', '[6]', 2), [
      {'rowid': 1003, 'distance': 4.0},
      {'rowid': 1002, 'distance': 9.0},
    ])
    
    db.close()
  
  def test_vss_stress(self):
    cur = db.cursor()
    
    execute_all(cur, 'create virtual table no_id_map using vss0(a(2) factory="Flat");')
    with self.assertRaisesRegex(sqlite3.OperationalError, ".*add_with_ids not implemented for this type of index.*"):
      execute_all(cur, """insert into no_id_map(rowid, a) select  100, json('[0, 1]')""")
      db.commit()
      

    execute_all(cur, 'create virtual table no_id_map2 using vss0(a(2) factory="Flat,IDMap");')
    execute_all(cur, "insert into no_id_map2(rowid, a) select  100, json('[0, 1]')")
    # fails because query references `a`, but cannot reconstruct the vector from the index bc only IDMap
    with self.assertRaisesRegex(sqlite3.OperationalError, ".*reconstruct not implemented for this type of index"):
      execute_all(cur, "select rowid, a from no_id_map2;")
    # but this suceeds, because only the rowid column is referenced
    execute_all(cur, "select rowid from no_id_map2;")
    
    with self.assertRaisesRegex(sqlite3.OperationalError, ".*could not parse index string invalid"):
      execute_all(cur, 'create virtual table t1 using vss0(a(2) factory="invalid");')
    
class TestCoverage(unittest.TestCase):                                      
  def test_coverage(self):                                                      
    test_methods = [method for method in dir(TestVss) if method.startswith('test_vss')]
    funcs_with_tests = set([x.replace("test_", "") for x in test_methods])
    for func in FUNCTIONS:
      self.assertTrue(func in funcs_with_tests, f"{func} does not have cooresponding test in {funcs_with_tests}")

if __name__ == '__main__':
    unittest.main()