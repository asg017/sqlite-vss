import sqlite3
import unittest
import time
import os
import tempfile

EXT_VSS_PATH="./dist/debug/vss0"
EXT_VECTOR_PATH="./dist/debug/vector0"

def connect(path=":memory:"):
  db = sqlite3.connect(path)

  db.enable_load_extension(True)

  db.execute("create temp table base_functions as select name from pragma_function_list")
  db.execute("create temp table base_modules as select name from pragma_module_list")
  db.load_extension(EXT_VECTOR_PATH)
  db.execute("create temp table vector_loaded_functions as select name from pragma_function_list where name not in (select name from base_functions) order by name")
  db.execute("create temp table vector_loaded_modules as select name from pragma_module_list where name not in (select name from base_modules) order by name")

  db.execute("drop table base_functions")
  db.execute("drop table base_modules")

  db.execute("create temp table base_functions as select name from pragma_function_list")
  db.execute("create temp table base_modules as select name from pragma_module_list")

  db.load_extension(EXT_VSS_PATH)

  db.execute("create temp table vss_loaded_functions as select name from pragma_function_list where name not in (select name from base_functions) order by name")
  db.execute("create temp table vss_loaded_modules as select name from pragma_module_list where name not in (select name from base_modules) order by name")

  db.row_factory = sqlite3.Row
  return db


db = connect()

def explain_query_plan(sql):
  return db.execute("explain query plan " + sql).fetchone()["detail"]

def execute_all(cursor, sql, args=None):
  if args is None: args = []
  results = cursor.execute(sql, args).fetchall()
  return list(map(lambda x: dict(x), results))

VSS_FUNCTIONS = [
  'vss_cosine_similarity',
  'vss_debug',
  'vss_distance_l1',
  'vss_distance_l2',
  'vss_distance_linf',
  'vss_fvec_add',
  'vss_fvec_sub',
  'vss_inner_product',
  'vss_memory_usage',
  'vss_range_search',
  'vss_range_search_params',
  'vss_search',
  'vss_search_params',
  'vss_version',
]

VSS_MODULES = [
  "vss0",
]
class TestVss(unittest.TestCase):
  def test_funcs(self):
    funcs = list(map(lambda a: a[0], db.execute("select name from vss_loaded_functions").fetchall()))
    self.assertEqual(funcs, VSS_FUNCTIONS)

  def test_modules(self):
    modules = list(map(lambda a: a[0], db.execute("select name from vss_loaded_modules").fetchall()))
    self.assertEqual(modules, VSS_MODULES)


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

  def test_vss_cosine_similarity(self):
    vss_cosine_similarity = lambda a, b: db.execute("select vss_cosine_similarity(json(?), json(?))", [a, b]).fetchone()[0]
    self.assertAlmostEqual(vss_cosine_similarity('[2, 1]', '[1, 2]'), 0.8)
    self.assertEqual(vss_cosine_similarity('[1, 1]', '[-1, 1]'), 0.0)

  def test_vss_fvec_add(self):
    vss_fvec_add = lambda a, b: db.execute("select vss_fvec_add(json(?), json(?))", [a, b]).fetchone()[0]
    self.assertEqual(vss_fvec_add('[0, 0]', '[0, 0]'), b'\x00\x00\x00\x00\x00\x00\x00\x00')
    self.assertEqual(vss_fvec_add('[-1, -1]', '[1, 1]'), b'\x00\x00\x00\x00\x00\x00\x00\x00')
    self.assertEqual(vss_fvec_add('[0, 0]', '[1, 1]'), b'\x00\x00\x80?\x00\x00\x80?')
    self.skipTest("TODO")

  def test_vss_fvec_sub(self):
    vss_fvec_sub = lambda a, b: db.execute("select vss_fvec_sub(json(?), json(?))", [a, b]).fetchone()[0]
    self.assertEqual(vss_fvec_sub('[0, 0]', '[0, 0]'), b'\x00\x00\x00\x00\x00\x00\x00\x00')
    self.assertEqual(vss_fvec_sub('[1, 1]', '[1, 1]'), b'\x00\x00\x00\x00\x00\x00\x00\x00')
    self.assertEqual(vss_fvec_sub('[0, 0]', '[1, 1]'), b'\x00\x00\x80\xbf\x00\x00\x80\xbf')
    self.skipTest("TODO")

  def test_vss_search(self):
    self.skipTest("TODO")

  def test_vss_search_params(self):
    self.skipTest("TODO")

  def test_vss_memory_usage(self):
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
      {'rowid': 0, 'length(idx)': 170},
      {'rowid': 1, 'length(idx)': 150}
    ])
    self.assertEqual(execute_all(cur, "select rowid from x_data"), [
      {"rowid": 1000},
      {"rowid": 1001},
      {"rowid": 1002},
      {"rowid": 1003},
      {"rowid": 1004},
    ])

    with self.subTest("delete + commit"):
      execute_all(cur, "delete from x where rowid = 1004")
      db.commit()

      self.assertEqual(execute_all(cur, "select rowid from x_data"), [
        {"rowid": 1000},
        {"rowid": 1001},
        {"rowid": 1002},
        {"rowid": 1003},
      ])

      self.assertEqual(execute_all(cur, "select rowid, length(idx) from x_index"), [
        {'rowid': 0, 'length(idx)': 154},
        {'rowid': 1, 'length(idx)': 138}
      ])

    with self.subTest("delete + rollback"):
      execute_all(cur, "delete from x where rowid = 1003")
      self.assertEqual(execute_all(cur, "select rowid from x_data"), [
        {"rowid": 1000},
        {"rowid": 1001},
        {"rowid": 1002},
      ])

      db.rollback()

      self.assertEqual(execute_all(cur, "select rowid from x_data"), [
        {"rowid": 1000},
        {"rowid": 1001},
        {"rowid": 1002},
        {"rowid": 1003},
      ])

      self.assertEqual(execute_all(cur, "select rowid, length(idx) from x_index"), [
        {'rowid': 0, 'length(idx)': 154},
        {'rowid': 1, 'length(idx)': 138}
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

    with self.assertRaisesRegex(sqlite3.OperationalError, 'Input query size doesn\'t match index dimensions: 0 != 1'):
      search('b', '[]', 2)

    with self.assertRaisesRegex(sqlite3.OperationalError, 'Input query size doesn\'t match index dimensions: 3 != 1'):
      search('b', '[0.1, 0.2, 0.3]', 2)

    with self.assertRaisesRegex(sqlite3.OperationalError, 'Limit must be greater than 0, got -1'):
      search('b', '[6]', -1)

    with self.assertRaisesRegex(sqlite3.OperationalError, 'Limit must be greater than 0, got 0'):
      search('b', '[6]', 0)

    self.assertEqual(range_search('a', '[0.5, 0.5]', 1), [
      {'rowid': 1000, 'distance': 0.5},
      {'rowid': 1002, 'distance': 0.5},
    ])
    self.assertEqual(range_search('b', '[2.5]', 1), [
      {'rowid': 1001, 'distance': 0.25},
      {'rowid': 1002, 'distance': 0.25},
    ])

    self.assertEqual(execute_all(cur, 'select rowid, a, b, distance from x'), [
      {'rowid': 1000, "a": b'\x00\x00\x00\x00\x00\x00\x80?', "b": b'\x00\x00\x80?', "distance": None},
      {'rowid': 1001, "a": b'\x00\x00\x00\x00\x00\x00\x80\xbf', "b": b'\x00\x00\x00@', "distance": None},
      {'rowid': 1002, "a": b'\x00\x00\x80?\x00\x00\x00\x00', "b": b'\x00\x00@@', "distance": None},
      {'rowid': 1003, "a": b'\x00\x00\x80\xbf\x00\x00\x00\x00', "b": b'\x00\x00\x80@', "distance": None},
    ])
    self.assertEqual(execute_all(cur, 'select rowid, vector_debug(a) as a, vector_debug(b) as b, distance from x'), [
      {'rowid': 1000, "a": "size: 2 [0.000000, 1.000000]",  "b": "size: 1 [1.000000]", "distance": None},
      {'rowid': 1001, "a": "size: 2 [0.000000, -1.000000]", "b": "size: 1 [2.000000]", "distance": None},
      {'rowid': 1002, "a": "size: 2 [1.000000, 0.000000]",  "b": "size: 1 [3.000000]", "distance": None},
      {'rowid': 1003, "a": "size: 2 [-1.000000, 0.000000]", "b": "size: 1 [4.000000]", "distance": None},
    ])

    with self.assertRaisesRegex(sqlite3.OperationalError, "UPDATE statements on vss0 virtual tables not supported yet."):
      execute_all(cur, 'update x set b = json(?) where rowid = ?', ['[444]', 1003])
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
  def test_vss0_persistent_stress(self):
    tf = tempfile.NamedTemporaryFile(delete=False)
    tf.close()

    # create a vss0 table with no data, then close it. When re-opening it, should work as expected
    db = connect(tf.name)
    db.execute("create virtual table x using vss0(a(2));")
    db.close()

    db = connect(tf.name)
    self.assertEqual(execute_all(db, "select rowid, length(idx) as length from x_index"), [
      {'rowid': 0, 'length': 90},
    ])
    db.execute("insert into x(rowid, a) select 1, json_array(1, 2)")
    db.commit()

    self.assertEqual(execute_all(db, "select rowid, length(idx) as length from x_index"), [
      {'rowid': 0, 'length': 106},
    ])

    self.assertEqual(execute_all(db, "select rowid, vector_debug(a) as a from x"), [
      {'rowid': 1, 'a': 'size: 2 [1.000000, 2.000000]'},
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

    with self.assertRaisesRegex(sqlite3.OperationalError, "unknown metric type: L3"):
      db.execute("create virtual table xx using vss0( a(2) metric_type=L3)")

    with self.assertRaisesRegex(sqlite3.OperationalError, "invalid metric_type value"):
      db.execute("create virtual table xx using vss0( a(2) metric_type=)")

  def test_vss_training(self):
    import random
    import json
    cur = db.cursor()
    execute_all(
      cur,
      'create virtual table with_training using vss0(a(4) factory="IVF10,Flat,IDMap2", b(4) factory="IVF10,Flat,IDMap2")'
    )
    data = list(map(lambda x: [random.uniform(0, 1), random.uniform(0, 1), random.uniform(0, 1), random.uniform(0, 1)], range(0, 1000)))
    execute_all(
      cur,
      """
        insert into with_training(operation, a, b)
          select
            'training',
            value,
            value
          from json_each(?)
      """,
      [json.dumps(data)]
    )
    db.commit()
    self.assertEqual(cur.execute('select count(*) from with_training').fetchone()[0], 0)

    execute_all(
      cur,
      """
        insert into with_training(rowid, a, b)
          select
            key,
            value,
            value
          from json_each(?)
      """,
      [json.dumps(data)]
    )
    self.assertEqual(cur.execute('select count(*) from with_training').fetchone()[0], 1000)
    db.commit()
    self.assertEqual(cur.execute('select count(*) from with_training').fetchone()[0], 1000)

  def test_vss0_issue_29_upsert(self):
    db = connect()
    execute_all(db , '')
    db.executescript("""
    create virtual table demo using vss0(a(2));

    insert into demo(rowid, a)
      values (1, '[1.0, 2.0]'), (2, '[2.0, 3.0]');
    """)

    self.assertEqual(
      execute_all(db, "select rowid, vector_debug(a) from demo;"),
      [
        {'rowid': 1, 'vector_debug(a)': 'size: 2 [1.000000, 2.000000]'},
        {'rowid': 2, 'vector_debug(a)': 'size: 2 [2.000000, 3.000000]'}
      ]
    )

    db.executescript("""
    delete from demo where rowid = 1;
    insert into demo(rowid, a) select 1, '[99.0, 99.0]';

    delete from demo where rowid = 2;
    insert into demo(rowid, a) select 2, '[299.0, 299.0]';
    """)

    # This is what used to fail, since we we're clearing deleted IDs properly
    self.assertEqual(
      execute_all(db, "select rowid, vector_debug(a) from demo;"),
      [
        {'rowid': 1, 'vector_debug(a)': 'size: 2 [99.000000, 99.000000]'},
        {'rowid': 2, 'vector_debug(a)': 'size: 2 [299.000000, 299.000000]'}
      ]
    )
    db.close()

  # Make sure tha VACUUMing a database with vss0 tables still works as expected
  def test_vss0_vacuum(self):
    cur = db.cursor()
    execute_all(cur, "create virtual table x using vss0(a(2));")
    execute_all(cur, """
      insert into x(rowid, a)
        select
          key + 1000,
          value
        from json_each(?);
      """, ["""
        [
          [1, 1],
          [2, 2],
          [3, 3]
        ]
        """])
    db.commit()

    db.execute("VACUUM;")

    self.assertEqual(
      execute_all(db, "select rowid, distance from x where vss_search(a, vss_search_params(?, ?))", ['[0, 0]', 1]),
      [{'distance': 2.0, 'rowid': 1000}]
    )

  def test_vss0_metric_type(self):
    cur = db.cursor()
    execute_all(
      cur,
      '''create virtual table vss_mts using vss0(
        ip(2) metric_type=INNER_PRODUCT,
        l1(2) metric_type=L1,
        l2(2) metric_type=L2,
        linf(2) metric_type=Linf,
        -- lp(2) metric_type=Lp,
        canberra(2) metric_type=Canberra,
        braycurtis(2) metric_type=BrayCurtis,
        jensenshannon(2) metric_type=JensenShannon
      )'''
    )
    idxs = list(map(lambda row: row[0], db.execute("select idx from vss_mts_index").fetchall()))

    # ensure all the indexes are IDMap2 ("IxM2")
    for idx in idxs:
      idx_type = idx[0:4]
      self.assertEqual(idx_type, b"IxM2")

    # manually tested until i ended up at 33 ¯\_(ツ)_/¯
    METRIC_TYPE_OFFSET = 33

    # values should match https://github.com/facebookresearch/faiss/blob/43d86e30736ede853c384b24667fc3ab897d6ba9/faiss/MetricType.h#L22
    self.assertEqual(idxs[0][METRIC_TYPE_OFFSET], 0) # ip
    self.assertEqual(idxs[1][METRIC_TYPE_OFFSET], 2) # l1
    self.assertEqual(idxs[2][METRIC_TYPE_OFFSET], 1) # l2
    self.assertEqual(idxs[3][METRIC_TYPE_OFFSET], 3) # linf
    #self.assertEqual(idxs[4][METRIC_TYPE_OFFSET], 4) # lp
    self.assertEqual(idxs[4][METRIC_TYPE_OFFSET], 20) # canberra
    self.assertEqual(idxs[5][METRIC_TYPE_OFFSET], 21) # braycurtis
    self.assertEqual(idxs[6][METRIC_TYPE_OFFSET], 22) # jensenshannon


    db.execute(
      "insert into vss_mts(rowid, ip, l1, l2, linf, /*lp,*/ canberra, braycurtis, jensenshannon) values (1, ?1,?1,?1,?1, /*?1,*/ ?1,?1,?2)",
      ["[4,1]", "[0.8, 0.2]"]
    )
    db.commit()

    def distance_of(metric_type, query):
      return db.execute(
       f"select distance from vss_mts where vss_search({metric_type}, vss_search_params(?1, 1))",
       [query]
      ).fetchone()[0]

    self.assertEqual(distance_of("ip",          "[0,0]"), 0.0)
    self.assertEqual(distance_of("l1",          "[0,0]"), 5.0)
    self.assertEqual(distance_of("l2",          "[0,0]"), 17.0)
    self.assertEqual(distance_of("linf",        "[0,0]"), 4.0)
    #self.assertEqual(distance_of("lp",          "[0,0]"), 2.0)
    self.assertEqual(distance_of("canberra",    "[0,0]"), 2.0)
    self.assertEqual(distance_of("braycurtis",  "[0,0]"), 1.0)

    # JS distance assumes L1 normalized input (a valid probability distribution)
    # additionally, faiss actually computes JS divergence and not JS distance
    self.assertAlmostEqual(distance_of("jensenshannon", "[0.33333333, 0.66666667]"), 0.1157735)

    self.assertEqual(distance_of("ip", "[2,2]"), 10.0)


VECTOR_FUNCTIONS = [
  'vector0',
  'vector_debug',
  'vector_from_blob',
  'vector_from_json',
  'vector_from_raw',
  'vector_length',
  'vector_to_blob',
  'vector_to_json',
  'vector_to_raw',
  'vector_value_at',
  'vector_version',
]

VECTOR_MODULES = ['vector_fvecs_each']
class TestVector(unittest.TestCase):
  def test_funcs(self):
    funcs = list(map(lambda a: a[0], db.execute("select name from vector_loaded_functions").fetchall()))
    self.assertEqual(funcs, VECTOR_FUNCTIONS)

  def test_modules(self):
    modules = list(map(lambda a: a[0], db.execute("select name from vector_loaded_modules").fetchall()))
    self.assertEqual(modules, VECTOR_MODULES)


  def test_vector_version(self):
    self.assertEqual(db.execute("select vector_version()").fetchone()[0][0], "v")

  def test_vector_debug(self):
    self.assertEqual(
      db.execute("select vector_debug(json('[]'))").fetchone()[0],
      "size: 0 []"
    )
    with self.assertRaisesRegex(sqlite3.OperationalError, "Value not a vector"):
      db.execute("select vector_debug(']')").fetchone()

  def test_vector0(self):
    self.assertEqual(db.execute("select vector0(null)").fetchone()[0], None)

  def test_vector_from_blob(self):
    self.assertEqual(
      db.execute("select vector_debug(vector_from_blob(vector_to_blob(vector_from_json(?))))", ["[0.1,0.2]"]).fetchone()[0],
      "size: 2 [0.100000, 0.200000]"
    )
    def raises_small_blob_header(input):
      with self.assertRaisesRegex(sqlite3.OperationalError, "Vector blob size less than header length"):
        db.execute("select vector_from_blob(?)", [input]).fetchall()

    raises_small_blob_header(b"")
    raises_small_blob_header(b"v")

    with self.assertRaisesRegex(sqlite3.OperationalError, "Blob not well-formatted vector blob"):
        db.execute("select vector_from_blob(?)", [b"V\x01\x00\x00\x00\x00"]).fetchall()

    with self.assertRaisesRegex(sqlite3.OperationalError, "Blob type not right"):
        db.execute("select vector_from_blob(?)", [b"v\x00\x00\x00\x00\x00"]).fetchall()

  def test_vector_to_blob(self):
    vector_to_blob = lambda x: db.execute("select vector_to_blob(vector_from_json(json(?)))", [x]).fetchone()[0]
    self.assertEqual(vector_to_blob("[]"), b"v\x01")
    self.assertEqual(vector_to_blob("[0.1]"), b"v\x01\xcd\xcc\xcc=")
    self.assertEqual(vector_to_blob("[0.1, 0]"), b"v\x01\xcd\xcc\xcc=\x00\x00\x00\x00")

  def test_vector_to_raw(self):
    vector_to_raw = lambda x: db.execute("select vector_to_raw(vector_from_json(json(?)))", [x]).fetchone()[0]
    self.assertEqual(vector_to_raw("[]"), None) # TODO why not b""
    self.assertEqual(vector_to_raw("[0.1]"), b"\xcd\xcc\xcc=")
    self.assertEqual(vector_to_raw("[0.1, 0]"), b"\xcd\xcc\xcc=\x00\x00\x00\x00")

  def test_vector_from_raw(self):
    vector_from_raw_blob = lambda x: db.execute("select vector_debug(vector_from_raw(?))", [x]).fetchone()[0]
    self.assertEqual(
      vector_from_raw_blob(b""),
      'size: 0 []'
    )
    self.assertEqual(
      vector_from_raw_blob(b"\x00\x00\x00\x00"),
      'size: 1 [0.000000]'
    )
    self.assertEqual(
      vector_from_raw_blob(b"\x00\x00\x00\x00\xcd\xcc\xcc="),
      'size: 2 [0.000000, 0.100000]'
    )

    with self.assertRaisesRegex(sqlite3.OperationalError, "Invalid raw blob length, blob must be divisible by 4"):
      vector_from_raw_blob(b"abc")

  def test_vector_from_json(self):
    vector_from_json = lambda x: db.execute("select vector_debug(vector_from_json(?))", [x]).fetchone()[0]
    self.assertEqual(vector_from_json('[0.1, 0.2, 0.3]'), "size: 3 [0.100000, 0.200000, 0.300000]")
    self.assertEqual(vector_from_json('[]'), "size: 0 []")
    with self.assertRaisesRegex(sqlite3.OperationalError, "input not valid json, or contains non-float data"):
      vector_from_json('')
    #db.execute("select vector_from_json(?)", [""]).fetchone()[0]

  def test_vector_to_json(self):
    vector_to_json = lambda x: db.execute("select vector_debug(vector_to_json(vector_from_json(json(?))))", [x]).fetchone()[0]
    self.assertEqual(vector_to_json('[0.1, 0.2, 0.3]'), "size: 3 [0.100000, 0.200000, 0.300000]")

  def test_vector_length(self):
    vector_length = lambda x: db.execute("select vector_length(vector_from_json(json(?)))", [x]).fetchone()[0]
    self.assertEqual(vector_length('[0.1, 0.2, 0.3]'), 3)
    self.assertEqual(vector_length('[0.1]'), 1)
    self.assertEqual(vector_length('[]'), 0)

  def test_vector_value_at(self):
    vector_value_at = lambda x, y: db.execute("select vector_value_at(vector_from_json(json(?)), ?)", [x, y]).fetchone()[0]
    self.assertAlmostEqual(vector_value_at('[0.1, 0.2, 0.3]', 0), 0.1)
    self.assertAlmostEqual(vector_value_at('[0.1, 0.2, 0.3]', 1), 0.2)
    self.assertAlmostEqual(vector_value_at('[0.1, 0.2, 0.3]', 2), 0.3)
    #self.assertAlmostEqual(vector_value_at('[0.1, 0.2, 0.3]', 3), 0.3)

class TestCoverage(unittest.TestCase):
  def test_coverage_vss(self):
    test_methods = [method for method in dir(TestVss) if method.startswith('test_vss')]
    funcs_with_tests = set([x.replace("test_", "") for x in test_methods])
    for func in VSS_FUNCTIONS:
      self.assertTrue(func in funcs_with_tests, f"{func} does not have cooresponding test in {funcs_with_tests}")

  def test_coverage_vector(self):
    test_methods = [method for method in dir(TestVector) if method.startswith('test_vector')]
    funcs_with_tests = set([x.replace("test_", "") for x in test_methods])
    for func in VECTOR_FUNCTIONS:
      self.assertTrue(func in funcs_with_tests, f"{func} does not have cooresponding test in {funcs_with_tests}")

if __name__ == '__main__':
    unittest.main()
