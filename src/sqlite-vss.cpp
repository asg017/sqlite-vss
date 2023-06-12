
#pragma region Includes and typedefs

#include <cstdio>
#include <cstdlib>
#include "sqlite-vss.h"

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <iostream>

#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFPQ.h>
#include <faiss/index_factory.h>
#include <faiss/index_io.h>
#include <faiss/utils/utils.h>
#include <faiss/impl/io.h>
#include <faiss/impl/AuxIndexStructures.h>
#include <faiss/impl/IDSelector.h>
#include <faiss/utils/distances.h>

#include "sqlite-vector.h"

// Simplifying syntax further down in file.
typedef std::unique_ptr<std::vector<float>> ptr_vec;

#define VSS_SEARCH_FUNCTION SQLITE_INDEX_CONSTRAINT_FUNCTION

#pragma endregion

#pragma region Types

struct vss_search_params {

  ptr_vec vector;
  sqlite3_int64 limit;
};

struct vss_insert {

  vss_insert(float data, faiss::idx_t id)
    : data(data), id(id) { }

  float data;
  faiss::idx_t id;
};

struct vss_index {

  vss_index(faiss::Index * faIndex)
    : faIndex(faIndex) { }

  std::vector<faiss::idx_t> removals;
  std::vector<vss_insert> inserts;
  faiss::Index * faIndex;
};

struct vss_index_vtab : public sqlite3_vtab {

  sqlite3 * db;
  vector0_api * vector_api;

  vss_index_vtab(
    const std::string & schema,
    const std::string & name,
    sqlite3 * db,
    vector0_api * vector_api)
    : schema(schema), name(name), db(db), vector_api(vector_api) {

    pModule = nullptr;
    nRef = 0;
    zErrMsg = nullptr;
  }

  virtual ~vss_index_vtab() { }

  // Name of the virtual table. Must be freed during disconnect
  std::string name;

  // Name of the schema the virtual table exists in. Must be freed during disconnect
  std::string schema;

  // Vector holding all the faiss Indices the vtab uses
  std::vector<vss_index> indexes;

  // Whether the current transaction is inserting data for at least 1 column
  bool isInsertData;
};

struct vss_index_cursor : public sqlite3_vtab_cursor {

  vss_index_vtab * table;

  vss_index_cursor(vss_index_vtab * table)
    : table(table), current(0), rowId(0) {

    pVtab = nullptr;
  }

  virtual ~vss_index_cursor() { }

  sqlite3_int64 current;
  sqlite3_int64 rowId;

  sqlite3_int64 limit;
  std::vector<faiss::idx_t> search_ids;
  std::vector<float> search_distances;
};

struct vss_index_column {

  vss_index_column(std::string & name, sqlite3_int64 dimensions)
    : name(name), dimensions(dimensions) { }

  std::string name;
  sqlite3_int64 dimensions;
};

#pragma endregion

#pragma region Shadow table helpers

static int vssIndexShadowName(const char *zName) {

  static const char * azName[] = {
    "index",
    "data"
  };

  for(auto i = 0; i < sizeof(azName) / sizeof(azName[0]); i++) {

    if(sqlite3_stricmp(zName, azName[i]) == 0)
      return 1;
  }
  return 0;
}

static int write_index_insert(
  faiss::Index * index,
  sqlite3 * db,
  const std::string & schema,
  const std::string & name,
  int rowId) {

  auto writer = faiss::VectorIOWriter();
  faiss::write_index(index, &writer);
  sqlite3_int64 size = writer.data.size();

  auto sql = sqlite3_mprintf("insert into \"%w\".\"%w_index\"(rowid, idx) values (?, ?)", schema.c_str(), name.c_str());

  sqlite3_stmt * stmt = nullptr;
  auto rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK || stmt == nullptr) {

    printf("error prepping stmt\n");
    sqlite3_free(sql);
    return SQLITE_ERROR;
  }

  rc = sqlite3_bind_int64(stmt, 1, rowId);
  if (rc != SQLITE_OK) {

    sqlite3_finalize(stmt);
    sqlite3_free(sql);
    return SQLITE_ERROR;
  }

  rc = sqlite3_bind_blob64(stmt, 2, writer.data.data(), size, SQLITE_TRANSIENT);
  if (rc != SQLITE_OK) {

    sqlite3_finalize(stmt);
    sqlite3_free(sql);
    return SQLITE_ERROR;
  }

  rc = sqlite3_step(stmt);

  sqlite3_finalize(stmt);
  sqlite3_free(sql);

  if(rc == SQLITE_DONE) {

    // INSERT was success, index wasn't written yet, all good to exit
    return SQLITE_OK;

  } else {

    printf("ooops!!\n");

    // INSERT failed, return error
    return SQLITE_ERROR;
  }
}

static int shadow_data_insert(
  sqlite3 * db,
  const std::string & schema,
  const std::string & name,
  sqlite3_int64 rowid,
  sqlite3_int64 * retRowid) {

  auto sql = sqlite3_mprintf("insert into \"%w\".\"%w_data\"(rowid, x) values (?, ?);", schema.c_str(), name.c_str());

  sqlite3_stmt * stmt = nullptr;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK || stmt == nullptr) {

    sqlite3_free(sql);
    return SQLITE_ERROR;
  }

  rc = sqlite3_bind_int64(stmt, 1, rowid);
  if (rc != SQLITE_OK) {

    sqlite3_finalize(stmt);
    sqlite3_free(sql);
    return SQLITE_ERROR;
  }

  rc = sqlite3_bind_null(stmt, 2);
  if (rc != SQLITE_OK) {

    sqlite3_finalize(stmt);
    sqlite3_free(sql);
    return SQLITE_ERROR;
  }

  if(sqlite3_step(stmt) != SQLITE_DONE) {

    sqlite3_finalize(stmt);
    sqlite3_free(sql);
    return SQLITE_ERROR;
  }

  if(retRowid != nullptr)
    *retRowid = sqlite3_last_insert_rowid(db);

  sqlite3_finalize(stmt);
  sqlite3_free(sql);

  return SQLITE_OK;
}

static int shadow_data_delete(
  sqlite3 * db,
  const std::string & schema,
  const std::string & name,
  sqlite3_int64 rowid) {

  auto sql = sqlite3_mprintf("delete from \"%w\".\"%w_data\" where rowid = ?", schema.c_str(), name.c_str());

  sqlite3_stmt * stmt;
  auto rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK || stmt == nullptr) {

    sqlite3_free(sql);
    return SQLITE_ERROR;
  }

  rc = sqlite3_bind_int64(stmt, 1, rowid);
  if (rc != SQLITE_OK) {

    sqlite3_finalize(stmt);
    sqlite3_free(sql);
    return SQLITE_ERROR;
  }

  if(sqlite3_step(stmt) != SQLITE_DONE) {

    sqlite3_finalize(stmt);
    sqlite3_free(sql);
    return SQLITE_ERROR;
  }

  sqlite3_finalize(stmt);
  sqlite3_free(sql);

  return SQLITE_OK;
}

static faiss::Index * read_index_select(
  sqlite3 * db,
  const std::string & name,
  int rowid) {

  auto sql = sqlite3_mprintf("select idx from \"%w_index\" where rowid = ?", name.c_str());

  sqlite3_stmt * stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

  if (rc != SQLITE_OK || stmt == nullptr) {

    sqlite3_free(sql);
    return nullptr;
  }

  sqlite3_bind_int64(stmt, 1, rowid);
  if (rc != SQLITE_OK) {

    sqlite3_finalize(stmt);
    sqlite3_free(sql);
    return nullptr;
  }

  if(sqlite3_step(stmt) != SQLITE_ROW) {

    sqlite3_finalize(stmt);
    sqlite3_free(sql);
    return nullptr;
  }

  auto index_data = sqlite3_column_blob(stmt, 0);
  int64_t n = sqlite3_column_bytes(stmt, 0);

  auto reader = faiss::VectorIOReader();
  std::copy((const uint8_t*) index_data, ((const uint8_t*)index_data) + n, std::back_inserter(reader.data));

  sqlite3_finalize(stmt);
  sqlite3_free(sql);

  return faiss::read_index(&reader);
}

static int create_shadow_tables(
  sqlite3 * db,
  const std::string & schema,
  const std::string & name,
  int n) {

  auto sql = sqlite3_mprintf("create table \"%w\".\"%w_index\"(idx)", schema.c_str(), name.c_str());

  int rc = sqlite3_exec(db, sql, nullptr, nullptr, nullptr);

  sqlite3_free(sql);

  if (rc != SQLITE_OK)
    return rc;

  sql = sqlite3_mprintf("create table \"%w\".\"%w_data\"(x);", schema.c_str(), name.c_str());
  rc = sqlite3_exec(db, sql, nullptr, nullptr, nullptr);

  sqlite3_free(sql);

  return rc;
}

static int drop_shadow_tables(sqlite3 * db, const std::string & name) {

  const std::string drops[2] = { "drop table \"%w_index\";", "drop table \"%w_data\";" };

  for(int i = 0; i < 2; i++) {

    auto sql = sqlite3_mprintf(drops[i].c_str(), name.c_str());
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, nullptr);

    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
      return SQLITE_ERROR;
    }
  }
  return SQLITE_OK;
}

#pragma endregion

#pragma region Search functions

void delVssSearchParams(void * p) {

  vss_search_params * self = (vss_search_params *)p;
  delete self;
}

static void vssSearchParamsFunc(
  sqlite3_context * context,
  int argc,
  sqlite3_value ** argv) {

  auto vector_api = (vector0_api *) sqlite3_user_data(context);

  std::unique_ptr<vss_search_params> params(new vss_search_params());

  params->vector = vector_api->xValueAsVector(argv[0]);
  params->limit = sqlite3_value_int64(argv[1]);

  if(params->vector == nullptr) {

    sqlite3_result_error(context, "1st argument is not a vector", -1);
    return;
  }

  if(params->limit == 0) {

    sqlite3_result_error(context, "You cannot select 0 records", -1);
    return;
  }

  sqlite3_result_pointer(context, params.release(), "vss0_searchparams", delVssSearchParams);
}

static void vssSearchFunc(
  sqlite3_context * context,
  int argc,
  sqlite3_value ** argv) { }

static int vssIndexFindFunction(
  sqlite3_vtab * pVtab,
  int nArg,
  const char * zName,
  void (** pxFunc)(sqlite3_context *, int, sqlite3_value **),
  void ** ppArg) {
  
  if(sqlite3_stricmp(zName, "vss_search") == 0){
    *pxFunc = vssSearchFunc;
    *ppArg = 0;
    return VSS_SEARCH_FUNCTION;
  }

  return 0;
};

#pragma endregion

#pragma region Init and construction functions

std::vector<vss_index_column> parse_constructor(int argc, const char * const * argv) {

  auto columns = std::vector<vss_index_column>();
  for(int i = 3; i < argc; i++) {

    std::string arg = std::string(argv[i]);

    std::size_t lparen = arg.find("(");
    std::size_t rparen = arg.find(")");
    if(lparen == std::string::npos || rparen == std::string::npos || lparen >= rparen) {
      return columns;
    }

    std::string name = arg.substr(0, lparen);
    std::string sDimensions = arg.substr(lparen + 1, rparen - lparen - 1);
    sqlite3_int64 dimensions = std::atoi(sDimensions.c_str());

    columns.push_back(vss_index_column(name, dimensions));
  }

  return columns;
}

static int init(
  sqlite3 * db,
  void * pAux,
  int argc,
  const char * const * argv,
  sqlite3_vtab ** ppVtab,
  char ** pzErr,
  bool isCreate) {

  sqlite3_vtab_config(db, SQLITE_VTAB_CONSTRAINT_SUPPORT, 1);

  auto str = sqlite3_str_new(nullptr);
  sqlite3_str_appendall(str, "create table x(distance hidden, operation hidden");
  auto columns = parse_constructor(argc, argv);

  if(columns.size() == 0) {
    *pzErr = sqlite3_mprintf("Error parsing constructor");
    return SQLITE_ERROR;
  }

  for (auto column = columns.begin(); column != columns.end(); ++column) {
    sqlite3_str_appendf(str, ", \"%w\"", column->name.c_str());
  }

  sqlite3_str_appendall(str, ")");
  auto sql = sqlite3_str_finish(str);
  auto rc = sqlite3_declare_vtab(db, sql);
  sqlite3_free(sql);

  if(rc != SQLITE_OK)
    return rc;

  auto table = new vss_index_vtab(argv[1], argv[2], db, static_cast<vector0_api *> (pAux));

  *ppVtab = table;

  if (isCreate) {

    for (auto column = columns.begin(); column != columns.end(); ++column) {

      try {

        auto index = faiss::index_factory(column->dimensions, "Flat,IDMap2");
        table->indexes.push_back(vss_index(index));

      } catch(faiss::FaissException & e) {

        *pzErr = sqlite3_mprintf("Error building index factory for %s: %s", column->name.c_str(), e.msg.c_str());
        return SQLITE_ERROR;
      }
    }

    create_shadow_tables(db, argv[1], argv[2], columns.size());

    // After shadow tables are created, write the initial index state to shadow _index
    for (int i = 0; i < table->indexes.size(); i++) {

      auto index = table->indexes.at(i);

      try {

        int rc = write_index_insert(index.faIndex, table->db, table->schema, table->name, i);
        if(rc != SQLITE_OK)
          return rc;

      } catch(faiss::FaissException & e) {
        return SQLITE_ERROR;
      }
    }

  } else {

    for(int i = 0; i < columns.size(); i++) {

      auto index = read_index_select(db, argv[2], i);

      // Index in shadow table should always be available, integrity check to avoid null pointer
      if (index == nullptr) {

        *pzErr = sqlite3_mprintf("Could not read index at position %d", i);
        return SQLITE_ERROR;
      }
      table->indexes.push_back(vss_index(index));
    }
  }

  table->isInsertData = false;

  return SQLITE_OK;
}

#pragma endregion

#pragma region Connect/disconnect functions

static int vssIndexCreate(
  sqlite3 * db,
  void * pAux,
  int argc,
  const char * const * argv,
  sqlite3_vtab ** ppVtab,
  char ** pzErr) {

  return init(db, pAux, argc, argv, ppVtab, pzErr, true);
}

static int vssIndexConnect(
  sqlite3 * db,
  void * pAux,
  int argc,
  const char * const * argv,
  sqlite3_vtab ** ppVtab,
  char ** pzErr) {

  return init(db, pAux, argc, argv, ppVtab, pzErr, false);
}

static int vssIndexDisconnect(sqlite3_vtab * pVtab) {

  auto table = static_cast<vss_index_vtab *>(pVtab);
  delete table;
  return SQLITE_OK;
}

static int vssIndexDestroy(sqlite3_vtab * pVtab) {

  auto table = static_cast<vss_index_vtab *>(pVtab);
  drop_shadow_tables(table->db, table->name);
  return vssIndexDisconnect(table);
}

static int vssIndexOpen(sqlite3_vtab * pVtab, sqlite3_vtab_cursor ** cursor) {

  auto table = static_cast<vss_index_vtab *>(pVtab);
  auto pCur = new vss_index_cursor(table);

  if(pCur == nullptr)
    return SQLITE_NOMEM;

  *cursor = pCur;

  return SQLITE_OK;
}

static int vssIndexClose(sqlite3_vtab_cursor * cursor) {

  auto self = static_cast<vss_index_cursor *>(cursor);
  delete self;
  return SQLITE_OK;
}

#pragma endregion

#pragma region Misc functions

static int vssIndexBestIndex(
  sqlite3_vtab * tab,
  sqlite3_index_info * pIdxInfo) {

  printf("best index, %d\n", pIdxInfo->nConstraint);

  int iSearchTerm = -1;
  int iXSearchColumn = -1;
  int iLimit = -1;

  for(int i = 0; i < pIdxInfo->nConstraint; i++) {

    auto constraint = pIdxInfo->aConstraint[i];

    printf("\t[%d] col=%d, op=%d \n", i, pIdxInfo->aConstraint[i].iColumn, pIdxInfo->aConstraint[i].op);

    if(!constraint.usable)
      continue;

    if(constraint.op == VSS_SEARCH_FUNCTION) {

      iSearchTerm = i;
      iXSearchColumn = constraint.iColumn;

    } else if(constraint.op == SQLITE_INDEX_CONSTRAINT_LIMIT) {

      iLimit = i;
    }
  }

  if(iSearchTerm >= 0) {

    pIdxInfo->idxNum = iXSearchColumn - 2;
    pIdxInfo->idxStr = (char*) "search";
    pIdxInfo->aConstraintUsage[iSearchTerm].argvIndex = 1;
    pIdxInfo->aConstraintUsage[iSearchTerm].omit = 1;

    if(iLimit >= 0) {

      pIdxInfo->aConstraintUsage[iLimit].argvIndex = 2;
      pIdxInfo->aConstraintUsage[iLimit].omit = 1;
    }

    pIdxInfo->estimatedCost = 300.0;
    pIdxInfo->estimatedRows = 10;
    return SQLITE_OK;
  }

  return SQLITE_ERROR;
}

/*
 * This method is called to "rewind" the cursor object back
 * to the first row of output.
 */
static int vssIndexFilter(
  sqlite3_vtab_cursor * pVtabCursor,
  int idxNum,
  const char * idxStr,
  int argc,
  sqlite3_value ** argv) {

  auto cursor = (vss_index_cursor *) pVtabCursor;

  auto params = static_cast<vss_search_params*> (sqlite3_value_pointer(argv[0], "vss0_searchparams"));
  cursor->limit = params->limit;

  auto index = cursor->table->indexes.at(idxNum).faIndex;

  if(params->vector->size() != index->d) {

    sqlite3_free(pVtabCursor->pVtab->zErrMsg);

    pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf(
      "Input query size doesn't match index dimensions: %ld != %ld",
      params->vector->size(),
      index->d);

    return SQLITE_ERROR;
  }

  if(cursor->limit <= 0) {

    sqlite3_free(pVtabCursor->pVtab->zErrMsg);
    pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf("LIMIT must be greater than 0, got %ld", cursor->limit);
    return SQLITE_ERROR;
  }

  cursor->search_distances = std::vector<float>(cursor->limit);
  cursor->search_ids = std::vector<faiss::idx_t>(cursor->limit);
  index->search(1, params->vector->data(), cursor->limit, cursor->search_distances.data(), cursor->search_ids.data());

  for (auto iter = cursor->search_distances.begin(); iter != cursor->search_distances.end(); ++iter) {
    std::cout << *iter << std::endl;
  }

  cursor->current = 0;
  printf("\tindex filter, %d\n", cursor->limit);
  return SQLITE_OK;
}

static int vssIndexNext(sqlite3_vtab_cursor * cur) {

  vss_index_cursor * pCur = (vss_index_cursor *) cur;

  pCur->current ++;
  return SQLITE_OK;
}

static int vssIndexRowid(sqlite3_vtab_cursor * cur, sqlite_int64 * pRowid) {

  auto pCur = static_cast<vss_index_cursor *> (cur);
  *pRowid = pCur->search_ids.at(pCur->current);

  return SQLITE_OK;
}

static int vssIndexEof(sqlite3_vtab_cursor * cur) {

  auto pCur = static_cast<vss_index_cursor *>(cur);

  return pCur->current >= pCur->limit || pCur->current >= pCur->search_ids.size();
}

/*
 * Return values of columns for the row at which the cursor
 * is currently pointing.
 */
static int vssIndexColumn(
  sqlite3_vtab_cursor * cur,
  sqlite3_context * ctx,
  int i) {

  auto pCur = static_cast<vss_index_cursor *> (cur);

  if(i == 0) {

    sqlite3_result_double(ctx, pCur->search_distances.at(pCur->current));

  } else if (i >= 2) {

    auto index = pCur->table->indexes.at(i - 2).faIndex;

    auto vector = ptr_vec(new std::vector<float>(index->d));
    sqlite3_int64 rowid;
    vssIndexRowid(cur, &rowid);

    try {

      index->reconstruct(rowid, vector->data());

    } catch(faiss::FaissException & e) {

      char * errmsg = (char *) sqlite3_mprintf("Error reconstructing vector. Full error: %s", e.msg.c_str());
      sqlite3_result_error(ctx, errmsg, -1);
      sqlite3_free(errmsg);
      return SQLITE_ERROR;
    }

    pCur->table->vector_api->xResultVector(ctx, vector);
  }
  return SQLITE_OK;
}

static int vssIndexUpdate(
  sqlite3_vtab * pVTab,
  int argc,
  sqlite3_value ** argv,
  sqlite_int64 * pRowid) {

  auto p = (vss_index_vtab *) pVTab;

  if (argc == 1 && sqlite3_value_type(argv[0]) != SQLITE_NULL) {

    // DELETE operation
    sqlite3_int64 rowid_to_delete = sqlite3_value_int64(argv[0]);
    int rc;

    if((rc = shadow_data_delete(p->db, p->schema, p->name, rowid_to_delete)) != SQLITE_OK) {
      return rc;
    }

    for(int i = 0; i < p->indexes.size(); i++) {
      p->indexes.at(i).removals.push_back(rowid_to_delete);
    }

  } else if (argc > 1 && sqlite3_value_type(argv[0]) == SQLITE_NULL) {

    // INSERT operation
    // If no operation we add it to the index
    bool noOperation = sqlite3_value_type(argv[3]) == SQLITE_NULL;

    if (noOperation) {

      ptr_vec vec;
      sqlite3_int64 rowid = sqlite3_value_int64(argv[1]);
      bool inserted_rowid = false;

      for(int i = 0; i < p->indexes.size(); i++) {

        if((vec = p->vector_api->xValueAsVector(argv[4 + i])) != nullptr) {

          if(!inserted_rowid) {
            sqlite_int64 retrowid;
            int rc = shadow_data_insert(p->db, p->schema, p->name, rowid, &retrowid);
            if (rc != SQLITE_OK) {
              return rc;
            }
            inserted_rowid = true;
          }

          for (auto idx = 0; idx < vec->size(); idx++) {
            p->indexes.at(i).inserts.push_back(vss_insert(vec->at(idx), rowid));
          }

          p->isInsertData = true;
          *pRowid = rowid;
        }
      }
    }
  } else {

    // UPDATE operations
    sqlite3_free(pVTab->zErrMsg);
    pVTab->zErrMsg = sqlite3_mprintf("UPDATE on vss0 virtual tables not supported yet.");
    return SQLITE_ERROR;
  }
  return SQLITE_OK;
}

#pragma endregion

#pragma region Entrypoint functions

static sqlite3_module vssIndexModule = {
  /* iVersion    */ 3,
  /* xCreate     */ vssIndexCreate,
  /* xConnect    */ vssIndexConnect,
  /* xBestIndex  */ vssIndexBestIndex,
  /* xDisconnect */ vssIndexDisconnect,
  /* xDestroy    */ vssIndexDestroy,
  /* xOpen       */ vssIndexOpen,
  /* xClose      */ vssIndexClose,
  /* xFilter     */ vssIndexFilter,
  /* xNext       */ vssIndexNext,
  /* xEof        */ vssIndexEof,
  /* xColumn     */ vssIndexColumn,
  /* xRowid      */ vssIndexRowid,
  /* xUpdate     */ vssIndexUpdate,
  /* xBegin      */ nullptr,
  /* xSync       */ nullptr,
  /* xCommit     */ nullptr,
  /* xRollback   */ nullptr,
  /* xFindMethod */ vssIndexFindFunction,
  /* xRename     */ nullptr,
  /* xSavepoint  */ nullptr,
  /* xRelease    */ nullptr,
  /* xRollbackTo */ nullptr,
  /* xShadowName */ vssIndexShadowName
};

vector0_api * vector0_api_from_db(sqlite3 * db) {

  vector0_api * pRet = nullptr;
  sqlite3_stmt * pStmt = nullptr;

  if(SQLITE_OK == sqlite3_prepare(db, "select vector0(?1)", -1, &pStmt, 0) ) {

    sqlite3_bind_pointer(pStmt, 1, (void*)&pRet, "vector0_api_ptr", nullptr);
    sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);
  }
  return pRet;
}

extern "C" {

  #ifdef _WIN32
  __declspec(dllexport)
  #endif

  int sqlite3_vss_init(sqlite3 * db, char ** pzErrMsg, const sqlite3_api_routines * pApi) {

    SQLITE_EXTENSION_INIT2(pApi);
    auto vector_api = vector0_api_from_db(db);

    if(vector_api == nullptr) {

      *pzErrMsg = sqlite3_mprintf("The vector0 extension must be registered before vss0.");
      return SQLITE_ERROR;
    }

    static const struct {
      char *zFName;
      int nArg;
      int flags;
      vector0_api * api;
      void (*xFunc)(sqlite3_context*,int,sqlite3_value**);
    } aFunc[] = {
      { (char*) "vss_search",               2, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS, vector_api, vssSearchFunc },
      { (char*) "vss_search_params",        2, 0,                                                     vector_api, vssSearchParamsFunc }
    };

    for(int i=0; i < sizeof(aFunc) / sizeof(aFunc[0]); i++) {

      auto rc = sqlite3_create_function_v2(
        db,
        aFunc[i].zFName,
        aFunc[i].nArg,
        aFunc[i].flags,
        aFunc[i].api,
        aFunc[i].xFunc,
        nullptr,
        nullptr,
        nullptr);

      if(rc != SQLITE_OK) {
        *pzErrMsg = sqlite3_mprintf("%s: %s", aFunc[i].zFName, sqlite3_errmsg(db));
        return rc;
      }
    }

    return sqlite3_create_module_v2(db, "vss0", &vssIndexModule, vector_api, nullptr /* vector_api is static immutable */);
  }
}

#pragma endregion
