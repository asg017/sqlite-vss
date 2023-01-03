#include <cstdio>
#include <cstdlib>
#include "sqlite3ext.h"

SQLITE_EXTENSION_INIT1

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>


#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFPQ.h>
#include <faiss/index_factory.h>
#include <faiss/index_io.h>
#include <faiss/utils/utils.h>
#include <faiss/impl/io.h>
#include <faiss/impl/AuxIndexStructures.h>



#pragma region work 


struct VecX {
  int64_t size;
  float * data;
};

void del(void*p) {
  //delete p;
}
static void faiss_vector_debug(sqlite3_context *context, int argc, sqlite3_value **argv) {
  VecX* v = (VecX*) sqlite3_value_pointer(argv[0], "vecx0");
  std::vector<float> vvv(v->data, v->data + v->size);
}
static void vector(sqlite3_context *context, int argc, sqlite3_value **argv) {
  std::vector<float> *v = new std::vector<float>();
  for(int i = 0; i < argc; i++) {
    v->push_back(sqlite3_value_double(argv[i]));
  }
  VecX *vv = new VecX();
  vv->size = v->size();
  vv->data = v->data();
  sqlite3_result_pointer(context, vv, "vecx0", del);
  
}
static void vector_print(sqlite3_context *context, int argc, sqlite3_value **argv) {
  VecX* v = (VecX*) sqlite3_value_pointer(argv[0], "vecx0");
  std::vector<float> vvv(v->data, v->data + v->size);
  sqlite3_str *str = sqlite3_str_new(NULL);
  for (float v:vvv) {
    sqlite3_str_appendf(str, "%f,", v);
  }
  int n = sqlite3_str_length(str);
  sqlite3_result_text(context,  sqlite3_str_finish(str), n, SQLITE_TRANSIENT);
}
static void vector_to_blob(sqlite3_context *context, int argc, sqlite3_value **argv) {
  VecX* v = (VecX*) sqlite3_value_pointer(argv[0], "vecx0");
  //std::vector<float> vvv(v->data, v->data + v->size);
  int n = sizeof(int64_t) + (v->size * sizeof(float));
  void *p = sqlite3_malloc(n);

  printf("n=%d\n", n);
  memset(p, 0, n);
  void* p_data = (void*)( ((char*)p ) + sizeof(int64_t));
  printf("p=%p, p_data=%p\n", p, p_data);
  memcpy(p, (void *) &v->size, sizeof(v->size));
  printf("a\n");
  memcpy(p_data, (void *) v->data, n - sizeof(int64_t));
  //memset(p, )

  sqlite3_result_blob(context, p, n, SQLITE_TRANSIENT);
  sqlite3_free(p);
}
static void vector_from_blob(sqlite3_context *context, int argc, sqlite3_value **argv) {
  void* blob = (VecX*) sqlite3_value_blob(argv[0]);
  //int64_t size = 
  
  //std::vector<float> vvv(v->data, v->data + v->size);
  //int n = sizeof(int64_t) + (vvv.size() * sizeof(float));
  //sqlite3_result_blob(context, b, n, )
}

static void add_training(sqlite3_context *context, int argc, sqlite3_value **argv) {
  std::vector<float> * training = (std::vector<float> *) sqlite3_user_data(context);
  VecX* v = (VecX*) sqlite3_value_pointer(argv[0], "vecx0");
  std::vector<float> vvv(v->data, v->data + v->size);
  //printf("before: %d \n", training->size());
  training->reserve(vvv.size() + distance(vvv.begin(), vvv.end()));
  training->insert(training->end(), vvv.begin(),vvv.end());
  //printf("after: %d \n", training->size());
  sqlite3_result_int(context, 1);
}

#pragma endregion

#pragma region vtab

// https://github.com/sqlite/sqlite/blob/master/src/json.c#L88-L89
#define JSON_SUBTYPE  74    /* Ascii for "J" */

#include <nlohmann/json.hpp>
using json = nlohmann::json;

static std::vector<float>* valueAsVector(sqlite3_value*value) {
    VecX* v = (VecX*) sqlite3_value_pointer(value, "vecx0");
    if (v!=NULL) return new std::vector<float>(v->data, v->data + v->size);

    if(sqlite3_value_subtype(value) == JSON_SUBTYPE) {
      std::vector<float> v; 
      json data = json::parse(sqlite3_value_text(value));
      data.get_to(v);
      return  new std::vector<float>(v);
    }
    return NULL;
    
}


struct VssSearchParams {
  std::vector<float> * vector;
  sqlite3_int64 k;
};

static void VssSearchParamsFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  std::vector<float> * vector = valueAsVector(argv[0]);
  sqlite3_int64 k = sqlite3_value_int64(argv[1]);
  VssSearchParams* params = new VssSearchParams();
  params->vector = vector;
  params->k = k;
  sqlite3_result_pointer(context, params, "vss0_searchparams", 0);
}

struct VssRangeSearchParams {
  std::vector<float> * vector;
  float distance;
};

static void VssRangeSearchParamsFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  std::vector<float> * vector = valueAsVector(argv[0]);
  float distance = sqlite3_value_double(argv[1]);
  VssRangeSearchParams* params = new VssRangeSearchParams();
  params->vector = vector;
  params->distance = distance;
  sqlite3_result_pointer(context, params, "vss0_rangesearchparams", 0);
}

static int write_index_insert(faiss::Index * index, sqlite3*db, char * name) {
  faiss::VectorIOWriter * w = new faiss::VectorIOWriter();
  faiss::write_index(index, w);
  sqlite3_int64 nIn = w->data.size();

  sqlite3_str *query = sqlite3_str_new(0);
  sqlite3_stmt *stmt;
  sqlite3_str_appendf(query, "insert into \"%w_index\"(data) values (?)", name);
  char * q = sqlite3_str_finish(query);
  int rc = sqlite3_prepare_v2(db, q, -1, &stmt, 0);
  if (rc != SQLITE_OK || stmt==0) {
    printf("error prepping stmt\n");
    return SQLITE_ERROR;
  }
  sqlite3_bind_blob64(stmt, 1, w->data.data(), nIn, sqlite3_free);
  if(sqlite3_step(stmt) != SQLITE_DONE) {
    printf("error inserting?\n");
    sqlite3_finalize(stmt);
    return SQLITE_ERROR;
  }
  sqlite3_free(q);
  sqlite3_finalize(stmt);
  return SQLITE_OK;
}


static int shadow_data_insert(sqlite3*db, char * name, sqlite3_int64 *rowid, sqlite3_int64 *retRowid) {
  sqlite3_stmt *stmt;
  sqlite3_str *query = sqlite3_str_new(0);
  if(rowid == NULL) {
    sqlite3_str_appendf(query, "insert into \"%w_data\"(x) values (?)", name);
    char * q = sqlite3_str_finish(query);
    int rc = sqlite3_prepare_v2(db, q, -1, &stmt, 0);
    if (rc != SQLITE_OK || stmt==0) {
      printf("error prepping stmt\n");
      return SQLITE_ERROR;
    }
    sqlite3_bind_null(stmt, 1);
    if(sqlite3_step(stmt) != SQLITE_DONE) {
      printf("error inserting?\n");
      sqlite3_finalize(stmt);
      return SQLITE_ERROR;
    }
    sqlite3_free(q);
  } else {
    sqlite3_str_appendf(query, "insert into \"%w_data\"(rowid, x) values (?, ?) returning rowid;", name);
    char * q = sqlite3_str_finish(query);
    int rc = sqlite3_prepare_v2(db, q, -1, &stmt, 0);
    if (rc != SQLITE_OK || stmt==0) {
      printf("error prepping stmt\n");
      return SQLITE_ERROR;
    }
    sqlite3_bind_int64(stmt, 1, *rowid);
    sqlite3_bind_null(stmt, 2);
    if(sqlite3_step(stmt) != SQLITE_ROW) {
      printf("error inserting, no row stepped? %s\n", sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      return SQLITE_ERROR;
    }
    *retRowid = sqlite3_column_int64(stmt, 0);
    sqlite3_free(q);
  }
  sqlite3_finalize(stmt);
  return SQLITE_OK;
}

static faiss::Index * read_index_select(sqlite3 * db, const char * name) {
  sqlite3_str *query = sqlite3_str_new(0);
  sqlite3_stmt *stmt;
  sqlite3_str_appendf(query, "select data from \"%w_index\"", name);
  char * q = sqlite3_str_finish(query);
  int rc = sqlite3_prepare_v2(db, q, -1, &stmt, 0);
  if (rc != SQLITE_OK || stmt==0) {
    printf("error prepping stmt: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return 0;
  }
  if(sqlite3_step(stmt) != SQLITE_ROW) {
    printf("connect no row??\n");
    sqlite3_finalize(stmt);
    return 0;
  }
  const void * index_data = sqlite3_column_blob(stmt, 0);
  int64_t n = sqlite3_column_bytes(stmt, 0);
  faiss::VectorIOReader * r = new faiss::VectorIOReader();
  std::copy((const uint8_t*) index_data, ((const uint8_t*)index_data) + n, std::back_inserter(r->data));
  sqlite3_free(q);
  sqlite3_finalize(stmt);
  return faiss::read_index(r);
}

static int create_shadow_tables(sqlite3 * db, const char * schema, const char * name) {
  const char* creates[2] = {"CREATE TABLE \"%w\".\"%w_index\"(data blob);", "CREATE TABLE \"%w\".\"%w_data\"(x);"};

  for(int i = 0; i < 2; i++) {
    const char * s = creates[i];
    sqlite3_str *p = sqlite3_str_new(db);
    sqlite3_str_appendf(p, s, schema, name);
    char * zCreate = sqlite3_str_finish(p);
    if( !zCreate ){
      return SQLITE_NOMEM;
    }
    int rc = sqlite3_exec(db, zCreate, 0, 0, 0);
    sqlite3_free(zCreate);
    if (rc != SQLITE_OK) return rc;
  }
  
  return SQLITE_OK;
}

static int drop_shadow_tables(sqlite3*db, char * name) {
  const char* drops[2] = {"drop table \"%w_index\";", "drop table \"%w_data\";"};
  for(int i = 0; i < 2; i++) {
    const char * s = drops[i];
    
    sqlite3_stmt *stmt;
    
    sqlite3_str *query = sqlite3_str_new(0);
    sqlite3_str_appendf(query, s, name);
    char * q = sqlite3_str_finish(query);

    int rc = sqlite3_prepare_v2(db, q, -1, &stmt, 0);
    if (rc != SQLITE_OK || stmt==0) {
      printf("error prepping stmt\n");
      return SQLITE_ERROR;
    }
    if(sqlite3_step(stmt) != SQLITE_DONE) {
      printf("error dropping?\n");
      sqlite3_finalize(stmt);
      return SQLITE_ERROR;
    }
    sqlite3_free(q);
    sqlite3_finalize(stmt);
    
  }
  return SQLITE_OK;
}

#define VSS_SEARCH_FUNCTION       SQLITE_INDEX_CONSTRAINT_FUNCTION
#define VSS_RANGE_SEARCH_FUNCTION SQLITE_INDEX_CONSTRAINT_FUNCTION + 1

typedef struct vss_index_vtab vss_index_vtab;
struct vss_index_vtab {
  sqlite3_vtab base;  /* Base class - must be first */
  sqlite3 * db;
  char * name;
  faiss::Index * index;
  int dimensions;
  std::vector<float> * training;
  bool isTraining;
  bool isInsertData;
};

enum QueryType {search, range_search, fullscan}; 

typedef struct vss_index_cursor vss_index_cursor;
struct vss_index_cursor {
  sqlite3_vtab_cursor base;  /* Base class - must be first */
  vss_index_vtab * table;

  sqlite3_int64 iCurrent;
  sqlite3_int64 iRowid;

  QueryType query_type;

  // for query_type == QueryType::search
  sqlite3_int64 k;
  std::vector<faiss::idx_t> *nns;
  std::vector<float> *dis;
  
  // for query_type == QueryType::range_search
  faiss::RangeSearchResult * range_search_result;

  // for query_type == QueryType::fullscan
  sqlite3_stmt * stmt;
  int step_result;
};

struct ConstructorParams {
  sqlite3_int64 dimensions;
  char * factory_description;
  std::vector<std::string> columns;
};

static ConstructorParams* parse_constructor(int argc, const char *const*argv) {
  // TODO
  return NULL;
}

static int init(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr,
  bool isCreate
) {
  sqlite3_vtab_config(db, SQLITE_VTAB_CONSTRAINT_SUPPORT, 1);
  int rc;
  rc = sqlite3_declare_vtab(db,"CREATE TABLE x(distance, vector hidden, k hidden, operation hidden)");
  #define VSS_INDEX_COLUMN_DISTANCE    0
  #define VSS_INDEX_COLUMN_VECTOR      1
  #define VSS_INDEX_COLUMN_K           2
  #define VSS_INDEX_COLUMN_OPERATION   3
  
  if( rc!=SQLITE_OK ) return 0;
  
  vss_index_vtab *pNew;
  pNew = (vss_index_vtab *) sqlite3_malloc( sizeof(*pNew) );
  *ppVtab = (sqlite3_vtab*)pNew;
  memset(pNew, 0, sizeof(*pNew));
  
  int dimensions = std::stoi(argv[3]);
  if (isCreate) {
    create_shadow_tables(db, argv[1], argv[2]);
    char * description = sqlite3_mprintf("%.*s", strlen(argv[4])-2, argv[4]+1);//(char * ) argv[4]+1;
    //printf("d=%d, desc=|%s|\n", dimensions, description);
    faiss::Index *index = faiss::index_factory(dimensions, description);
    pNew->index = index; 
  } else {
    pNew->index = read_index_select(db, argv[2]);
  }

  pNew->training = new std::vector<float>();
  pNew->isTraining = false;
  pNew->isInsertData = false;
  pNew->name = sqlite3_mprintf("%s", argv[2]);
  pNew->dimensions = dimensions;
  pNew->db = db;
    
  return SQLITE_OK;
}

static int vssIndexCreate(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  return init(db, pAux, argc, argv, ppVtab, pzErr, true);
}

static int vssIndexConnect(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  return init(db, pAux, argc, argv, ppVtab, pzErr, false);
}

static int vssIndexDisconnect(sqlite3_vtab *pVtab){
  vss_index_vtab *p = (vss_index_vtab*)pVtab;
  sqlite3_free(p);
  return SQLITE_OK;
}

static int vssIndexDestroy(sqlite3_vtab *pVtab){
  vss_index_vtab *p = (vss_index_vtab*)pVtab;
  drop_shadow_tables(p->db, p->name);
  sqlite3_free(p);
  return SQLITE_OK;
}

static int vssIndexOpen(sqlite3_vtab *pVtab, sqlite3_vtab_cursor **ppCursor){
  vss_index_cursor *pCur;
  vss_index_vtab *p = (vss_index_vtab*)pVtab;
  pCur = (vss_index_cursor *) sqlite3_malloc( sizeof(*pCur) );
  if( pCur==0 ) return SQLITE_NOMEM;
  memset(pCur, 0, sizeof(*pCur));

  *ppCursor = &pCur->base;
  pCur->table = p;
  //pCur->nns = new std::vector<faiss::idx_t>(5);
  //pCur->dis = new std::vector<float>(5);
  
  return SQLITE_OK;
}

static int vssIndexClose(sqlite3_vtab_cursor *cur){
  vss_index_cursor *pCur = (vss_index_cursor*)cur;
  sqlite3_free(pCur);
  return SQLITE_OK;
}

static int vssIndexBestIndex(
  sqlite3_vtab *tab,
  sqlite3_index_info *pIdxInfo
){
  int iSearchTerm = -1;
  int iRangeSearchTerm = -1;
  //printf("best index, %d\n", pIdxInfo->nConstraint);

  for(int i = 0; i < pIdxInfo->nConstraint; i++) {
    auto constraint = pIdxInfo->aConstraint[i];
    //printf("\t[%d] col=%d, op=%d", i, pIdxInfo->aConstraint[i].iColumn, pIdxInfo->aConstraint[i].op);
    if(sqlite3_vtab_in(pIdxInfo, i, -1)) {
      //printf(" [IN POSSIBLE]");
    }
    //printf("\n");


    if(!constraint.usable) continue;
    if(constraint.op == VSS_SEARCH_FUNCTION) {
      iSearchTerm = i;
    }
    if(constraint.op == VSS_RANGE_SEARCH_FUNCTION) {
      iRangeSearchTerm = i;
    } 
  }
  if(iSearchTerm >=0) {
    pIdxInfo->idxNum = 1;
    pIdxInfo->idxStr = (char*) "search";
    pIdxInfo->aConstraintUsage[iSearchTerm].argvIndex = 1;
    pIdxInfo->aConstraintUsage[iSearchTerm].omit = 1;
    pIdxInfo->estimatedCost = 300.0;
    pIdxInfo->estimatedRows = 10;
    return SQLITE_OK;
  }
  if(iRangeSearchTerm >=0) {
    pIdxInfo->idxNum = 2;
    pIdxInfo->idxStr = (char*) "range_search";
    pIdxInfo->aConstraintUsage[iRangeSearchTerm].argvIndex = 1;
    pIdxInfo->aConstraintUsage[iRangeSearchTerm].omit = 1;
    pIdxInfo->estimatedCost = 300.0;
    pIdxInfo->estimatedRows = 10;
    return SQLITE_OK;
  }
  pIdxInfo->idxNum = 3;
  pIdxInfo->idxStr = (char*)"fullscan";
  pIdxInfo->estimatedCost = 3000000.0;
  pIdxInfo->estimatedRows = 100000;
  return SQLITE_OK;
}

static int vssIndexFilter(
  sqlite3_vtab_cursor *pVtabCursor, 
  int idxNum, const char *idxStr,
  int argc, sqlite3_value **argv
){
  vss_index_cursor *pCur = (vss_index_cursor *)pVtabCursor;  
  if (strcmp(idxStr, "search")==0) {
    pCur->query_type = QueryType::search;
    VssSearchParams* params = (VssSearchParams*) sqlite3_value_pointer(argv[0], "vss0_searchparams");
    int nq = 1;
    pCur->dis  = new std::vector<float>(params->k * nq);
    pCur->nns  = new std::vector<faiss::idx_t>(params->k * nq);
    pCur->table->index->search(nq, params->vector->data(), params->k, pCur->dis->data(), pCur->nns->data());
    pCur->k = params->k;
    
  }
  else if (strcmp(idxStr, "range_search")==0) {
  pCur->query_type = QueryType::range_search;
    VssRangeSearchParams* params = (VssRangeSearchParams*) sqlite3_value_pointer(argv[0], "vss0_rangesearchparams");
    int nq = 1;
    std::vector<faiss::idx_t> nns(params->distance * nq);
    faiss::RangeSearchResult * result = new faiss::RangeSearchResult(nq, true);
    //pCur->k = params->k;
    pCur->table->index->range_search(nq, params->vector->data(), params->distance, result);
    pCur->range_search_result = result;
    
  }else if (strcmp(idxStr, "fullscan")==0) {
    pCur->query_type = QueryType::fullscan;
    sqlite3_stmt* stmt;
    int res = sqlite3_prepare_v2(pCur->table->db, sqlite3_mprintf("select rowid from \"%w_data\"", pCur->table->name), -1, &pCur->stmt, NULL);
    if(res != SQLITE_OK) return res;
    pCur->step_result = sqlite3_step(pCur->stmt);
  }
  else {
    if(pVtabCursor->pVtab->zErrMsg != 0) sqlite3_free(pVtabCursor->pVtab->zErrMsg);
    pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf("%s %s", "vssIndexFilter error: unhandled idxStr", idxStr);
    return SQLITE_ERROR;
  }

  pCur->iCurrent = 0;
  return SQLITE_OK;
}

static int vssIndexNext(sqlite3_vtab_cursor *cur){
  vss_index_cursor *pCur = (vss_index_cursor*)cur;
  switch(pCur->query_type) {
    case QueryType::search:
    case QueryType::range_search: {
      pCur->iCurrent++;
      break;
    }
    case QueryType::fullscan: {
      pCur->step_result = sqlite3_step(pCur->stmt);
    }
  }
  
  return SQLITE_OK;
}
static int vssIndexRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid){
  vss_index_cursor *pCur = (vss_index_cursor*)cur;
  switch(pCur->query_type) {
    case QueryType::search: {
      *pRowid = pCur->nns->at(pCur->iCurrent);
      break;
    }
    case QueryType::range_search: {
      *pRowid = pCur->range_search_result->labels[pCur->iCurrent];
      break;
    }
    case QueryType::fullscan: {
      *pRowid = sqlite3_column_int64(pCur->stmt, 0);
      break;
    }
  }
  return SQLITE_OK;
}

static int vssIndexEof(sqlite3_vtab_cursor *cur){
  vss_index_cursor *pCur = (vss_index_cursor*)cur;
  switch(pCur->query_type) {
    case QueryType::search: {
      return pCur->iCurrent >= pCur->k || (pCur->nns->at(pCur->iCurrent) == -1);
    }
    case QueryType::range_search: {      
      return pCur->iCurrent >= pCur->range_search_result->lims[1]; 
    }
    case QueryType::fullscan: {
      return pCur->step_result != SQLITE_ROW;
    }
  }
  return 1;
}

static int vssIndexColumn(
  sqlite3_vtab_cursor *cur,
  sqlite3_context *ctx,
  int i
){
  vss_index_cursor *pCur = (vss_index_cursor*)cur;
  switch( i ){
    case VSS_INDEX_COLUMN_DISTANCE: {
      switch(pCur->query_type) {
        case QueryType::search: {
          sqlite3_result_double(ctx, pCur->dis->at(pCur->iCurrent));
          break;
        }
        case QueryType::range_search: {
          sqlite3_result_double(ctx, pCur->range_search_result->distances[pCur->iCurrent]);
          break;
        }
        case QueryType::fullscan: {
          break;
        }
      }
      break;
    }
    case VSS_INDEX_COLUMN_VECTOR: {
      try {
        sqlite3_int64 rowid;
        vssIndexRowid(cur, &rowid);
        std::vector<float> v(pCur->table->dimensions);
        pCur->table->index->reconstruct(rowid, v.data());
        json j = json(v);
        sqlite3_result_text(ctx, j.dump().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_result_subtype(ctx, JSON_SUBTYPE);
      }
      catch(const std::exception& ex) {
        printf("%s\n", ex.what());
        sqlite3_result_null(ctx);
      }
    }
      
  }
  return SQLITE_OK;
}


static int vssIndexBegin(sqlite3_vtab *tab) {
  //printf("BEGIN\n");
  return SQLITE_OK;
}

static int vssIndexSync(sqlite3_vtab *tab) {
  //printf("SYNC\n");
  return SQLITE_OK;
}

static int vssIndexCommit(sqlite3_vtab *pVTab) {
  vss_index_vtab *p = (vss_index_vtab*)pVTab;
  //printf("COMMIT %d %d\n", p->isTraining, p->isInsertData);
  if(p->isTraining) {
    //printf("TRAINING %lu\n", p->training->size());
    p->index->train(p->training->size() / p->dimensions, p->training->data());
    p->isTraining = false;
  }
  if(p->isInsertData) {
    //printf("WRITING INDEX\n");
    p->isInsertData = false;
    return write_index_insert(p->index, p->db, p->name);
  }
  return SQLITE_OK;
}

static int vssIndexRollback(sqlite3_vtab *tab) {
  //printf("ROLLBACK\n");
  return SQLITE_OK;
}

static int vssIndexUpdate(
  sqlite3_vtab *pVTab,
  int argc,
  sqlite3_value **argv,
  sqlite_int64 *pRowid
) {
  vss_index_vtab *p = (vss_index_vtab*)pVTab;
  if (argc ==1 && sqlite3_value_type(argv[0]) != SQLITE_NULL) {
    printf("xUpdate DELETE \n");
  }
  else if (argc > 1 && sqlite3_value_type(argv[0])== SQLITE_NULL) {
    // if no operation, we adding it to the index
    bool noOperation = sqlite3_value_type(argv[2+VSS_INDEX_COLUMN_OPERATION]) == SQLITE_NULL;
    if (noOperation) {
      std::vector<float>* vec;
      sqlite3_int64 rowid = sqlite3_value_int64(argv[1]);
      if ( (vec = valueAsVector(argv[2+VSS_INDEX_COLUMN_VECTOR])) != NULL ) {
        sqlite_int64 retrowid;
        shadow_data_insert(p->db, p->name, &rowid, &retrowid);
        p->index->add_with_ids(1, vec->data(), &rowid);
        p->isInsertData = true;
      }
      
    } else {
      std::string operation ((char *) sqlite3_value_text(argv[2+VSS_INDEX_COLUMN_OPERATION]));
      if(operation.compare("training") == 0) {
        std::vector<float>* vec;
        if ( (vec = valueAsVector(argv[2+VSS_INDEX_COLUMN_VECTOR])) != NULL ) {
          p->training->reserve(vec->size() + distance(vec->begin(), vec->end()));
          p->training->insert(p->training->end(), vec->begin(),vec->end());
          p->isTraining = true;
        }

        //VecX* v = (VecX*) sqlite3_value_pointer(argv[2+VSS_INDEX_COLUMN_VECTOR], "vecx0");
        //std::vector<float> vvv(v->data, v->data + v->size);
        
      
      } 
      else {
        printf("unknown operation\n");
        return SQLITE_ERROR;
      }
    }
  }
  else {
    //printf("xUpdate UNKNOWN \n");
  }
  return SQLITE_OK;
}


static void vssSearchFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  printf("search?\n");
}
static void faissMemoryUsageFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  sqlite3_result_int64(context, faiss::get_mem_usage_kb());
}
static void vssRangeSearchFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  printf("range search?\n");
}

static int vssIndexFindFunction(
  sqlite3_vtab *pVtab,
  int nArg,
  const char *zName,
  void (**pxFunc)(sqlite3_context*,int,sqlite3_value**),
  void **ppArg
) {
  //printf("find function. %d %s %s \n", nArg, zName, (char *) sqlite3_version);
  if( sqlite3_stricmp(zName, "vss_search")==0 ){
    *pxFunc = vssSearchFunc;
    *ppArg = 0;
    return VSS_SEARCH_FUNCTION;
  }
  if( sqlite3_stricmp(zName, "vss_range_search")==0 ){
    *pxFunc = vssRangeSearchFunc;
    *ppArg = 0;
    return VSS_RANGE_SEARCH_FUNCTION;
  }
  return 0;
};

static int vssIndexShadowName(const char *zName){
  static const char *azName[] = {
    "index",
    "data"
  };
  unsigned int i;
  for(i=0; i<sizeof(azName)/sizeof(azName[0]); i++){
    if( sqlite3_stricmp(zName, azName[i])==0 ) return 1;
  }
  return 0;
}

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
  /* xBegin      */ vssIndexBegin,
  /* xSync       */ vssIndexSync,
  /* xCommit     */ vssIndexCommit,
  /* xRollback   */ vssIndexRollback,
  /* xFindMethod */ vssIndexFindFunction,
  /* xRename     */ 0,
  /* xSavepoint  */ 0,
  /* xRelease    */ 0,
  /* xRollbackTo */ 0,
  /* xShadowName */ vssIndexShadowName
};

#pragma endregion

#pragma region entrypoint
void cleanup(void *p){}

extern "C" {
  #ifdef _WIN32
  __declspec(dllexport)
  #endif
  int sqlite3_extension_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    /*sqlite3_create_function_v2(db, "faiss_memory_usage", 0, 0, 0, faissMemoryUsageFunc, 0, 0, 0);
    sqlite3_create_function_v2(db, "faiss_vector_debug", 1, 0, 0, faiss_vector_debug, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector", -1, 0, 0, vector, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector_print", 1, 0, 0, vector_print, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector_to_blob", 1, 0, 0, vector_to_blob, 0, 0, 0);
    */
    sqlite3_create_function_v2(db, "vss_search", 2, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, vssSearchFunc, 0, 0, 0);
    sqlite3_create_function_v2(db, "vss_search_params", 2, 0, 0, VssSearchParamsFunc, 0, 0, 0);
    sqlite3_create_function_v2(db, "vss_range_search", 2, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, vssRangeSearchFunc, 0, 0, 0);
    sqlite3_create_function_v2(db, "vss_range_search_params", 2, 0, 0, VssRangeSearchParamsFunc, 0, 0, 0);
    
    
    sqlite3_create_module_v2  (db, "vss_index", &vssIndexModule, 0, 0);
    return 0;
  }
}

#pragma endregion