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


struct FaissSearchParams {
  std::vector<float> * vector;
  sqlite3_int64 k;
};

static void faissSearchParamsFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  std::vector<float> * vector = valueAsVector(argv[0]);
  sqlite3_int64 k = sqlite3_value_int64(argv[1]);
  FaissSearchParams* params = new FaissSearchParams();
  params->vector = vector;
  params->k = k;
  sqlite3_result_pointer(context, params, "faiss0_searchparams", 0);
}

struct FaissRangeSearchParams {
  std::vector<float> * vector;
  float distance;
};

static void faissRangeSearchParamsFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  std::vector<float> * vector = valueAsVector(argv[0]);
  float distance = sqlite3_value_double(argv[1]);
  FaissRangeSearchParams* params = new FaissRangeSearchParams();
  params->vector = vector;
  params->distance = distance;
  sqlite3_result_pointer(context, params, "faiss0_rangesearchparams", 0);
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

static int drop_index(sqlite3*db, char * name) {
  sqlite3_str *query = sqlite3_str_new(0);
  sqlite3_stmt *stmt;
  sqlite3_str_appendf(query, "drop table \"%w_index\";", name);
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
  sqlite3_str *p = sqlite3_str_new(db);
  sqlite3_str_appendf(p, "CREATE TABLE \"%w\".\"%w_index\"(data blob);", schema, name);
  char * zCreate = sqlite3_str_finish(p);
  if( !zCreate ){
    return SQLITE_NOMEM;
  }
  int rc = sqlite3_exec(db, zCreate, 0, 0, 0);
  sqlite3_free(zCreate);
  return rc;
}

#define FAISS_SEARCH_FUNCTION       SQLITE_INDEX_CONSTRAINT_FUNCTION
#define FAISS_RANGE_SEARCH_FUNCTION SQLITE_INDEX_CONSTRAINT_FUNCTION + 1

typedef struct faissIndex_vtab faissIndex_vtab;
struct faissIndex_vtab {
  sqlite3_vtab base;  /* Base class - must be first */
  sqlite3 * db;
  char * name;
  faiss::Index * index;
  int dimensions;
  std::vector<float> * training;
  bool isTraining;
  bool isInsertData;
};

enum QueryType {search, range_search}; 

typedef struct faissIndex_cursor faissIndex_cursor;
struct faissIndex_cursor {
  sqlite3_vtab_cursor base;  /* Base class - must be first */
  faissIndex_vtab * table;

  sqlite3_int64 iCurrent;
  sqlite3_int64 iRowid;

  QueryType query_type;

  // for query_type == QueryType::search
  sqlite3_int64 k;
  std::vector<faiss::idx_t> *nns;
  std::vector<float> *dis;
  
  // for query_type == QueryType::range_search
  faiss::RangeSearchResult * range_search_result;
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
  #define FAISSINDEX_COLUMN_DISTANCE    0
  #define FAISSINDEX_COLUMN_VECTOR      1
  #define FAISSINDEX_COLUMN_K           2
  #define FAISSINDEX_COLUMN_OPERATION   3
  
  if( rc!=SQLITE_OK ) return 0;
  
  faissIndex_vtab *pNew;
  pNew = (faissIndex_vtab *) sqlite3_malloc( sizeof(*pNew) );
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

static int faissIndexCreate(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  return init(db, pAux, argc, argv, ppVtab, pzErr, true);
}

static int faissIndexConnect(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  return init(db, pAux, argc, argv, ppVtab, pzErr, false);
}

static int faissIndexDisconnect(sqlite3_vtab *pVtab){
  faissIndex_vtab *p = (faissIndex_vtab*)pVtab;
  sqlite3_free(p);
  return SQLITE_OK;
}

static int faissIndexDestroy(sqlite3_vtab *pVtab){
  faissIndex_vtab *p = (faissIndex_vtab*)pVtab;
  drop_index(p->db, p->name);
  sqlite3_free(p);
  return SQLITE_OK;
}

static int faissIndexOpen(sqlite3_vtab *pVtab, sqlite3_vtab_cursor **ppCursor){
  faissIndex_cursor *pCur;
  faissIndex_vtab *p = (faissIndex_vtab*)pVtab;
  pCur = (faissIndex_cursor *) sqlite3_malloc( sizeof(*pCur) );
  if( pCur==0 ) return SQLITE_NOMEM;
  memset(pCur, 0, sizeof(*pCur));

  *ppCursor = &pCur->base;
  pCur->table = p;
  //pCur->nns = new std::vector<faiss::idx_t>(5);
  //pCur->dis = new std::vector<float>(5);
  
  return SQLITE_OK;
}

static int faissIndexClose(sqlite3_vtab_cursor *cur){
  faissIndex_cursor *pCur = (faissIndex_cursor*)cur;
  sqlite3_free(pCur);
  return SQLITE_OK;
}

static int faissIndexBestIndex(
  sqlite3_vtab *tab,
  sqlite3_index_info *pIdxInfo
){
  pIdxInfo->estimatedCost = (double)10;
  pIdxInfo->estimatedRows = 10;
  //printf("best index, %d\n", pIdxInfo->nConstraint);

  for(int i = 0; i < pIdxInfo->nConstraint; i++) {
    auto constraint = pIdxInfo->aConstraint[i];
    //printf("\t[%d] col=%d, op=%d", i, pIdxInfo->aConstraint[i].iColumn, pIdxInfo->aConstraint[i].op);
    if(sqlite3_vtab_in(pIdxInfo, i, -1)) {
      //printf(" [IN POSSIBLE]");
    }
    //printf("\n");


    if(!constraint.usable) continue;
    if(constraint.op == FAISS_SEARCH_FUNCTION) {
      pIdxInfo->idxStr = (char *)   "search";
      pIdxInfo->aConstraintUsage[i].argvIndex = 1;
      pIdxInfo->aConstraintUsage[i].omit = true;
      return SQLITE_OK;
    }
    if(constraint.op == FAISS_RANGE_SEARCH_FUNCTION) {
      pIdxInfo->idxStr = (char *) "range_search";
      pIdxInfo->aConstraintUsage[i].argvIndex = 1;
      pIdxInfo->aConstraintUsage[i].omit = true;
      return SQLITE_OK;
    } 
  }
  return SQLITE_CONSTRAINT;
}

static int faissIndexFilter(
  sqlite3_vtab_cursor *pVtabCursor, 
  int idxNum, const char *idxStr,
  int argc, sqlite3_value **argv
){
  faissIndex_cursor *pCur = (faissIndex_cursor *)pVtabCursor;
  
  if (strcmp(idxStr, "search")==0) {
    FaissSearchParams* params = (FaissSearchParams*) sqlite3_value_pointer(argv[0], "faiss0_searchparams");
    int nq = 1;
    pCur->dis  = new std::vector<float>(params->k * nq);
    pCur->nns  = new std::vector<faiss::idx_t>(params->k * nq);
    pCur->table->index->search(nq, params->vector->data(), params->k, pCur->dis->data(), pCur->nns->data());
    pCur->k = params->k;
    pCur->query_type = QueryType::search;
  }
  else if (strcmp(idxStr, "range_search")==0) {
    FaissRangeSearchParams* params = (FaissRangeSearchParams*) sqlite3_value_pointer(argv[0], "faiss0_rangesearchparams");
    int nq = 1;
    std::vector<faiss::idx_t> nns(params->distance * nq);
    faiss::RangeSearchResult * result = new faiss::RangeSearchResult(nq, true);
    //pCur->k = params->k;
    pCur->table->index->range_search(nq, params->vector->data(), params->distance, result);
    pCur->range_search_result = result;
    pCur->query_type = QueryType::range_search;
  }

  pCur->iCurrent = 0;
  return SQLITE_OK;
}

static int faissIndexNext(sqlite3_vtab_cursor *cur){
  faissIndex_cursor *pCur = (faissIndex_cursor*)cur;
  pCur->iCurrent++;
  return SQLITE_OK;
}
static int faissIndexRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid){
  faissIndex_cursor *pCur = (faissIndex_cursor*)cur;
  switch(pCur->query_type) {
    case QueryType::search: {
      *pRowid = pCur->nns->at(pCur->iCurrent);
      break;
    }
    case QueryType::range_search: {
      *pRowid = pCur->range_search_result->labels[pCur->iCurrent];
      break;
    }
  }
  return SQLITE_OK;
}

static int faissIndexEof(sqlite3_vtab_cursor *cur){
  faissIndex_cursor *pCur = (faissIndex_cursor*)cur;
  switch(pCur->query_type) {
    case QueryType::search: {
      return pCur->iCurrent >= pCur->k || (pCur->nns->at(pCur->iCurrent) == -1);
    }
    case QueryType::range_search: {      
      return pCur->iCurrent >= pCur->range_search_result->lims[1]; 
    }
  }
  return 1;
}

static int faissIndexColumn(
  sqlite3_vtab_cursor *cur,
  sqlite3_context *ctx,
  int i
){
  faissIndex_cursor *pCur = (faissIndex_cursor*)cur;
  switch( i ){
    case FAISSINDEX_COLUMN_DISTANCE: {
      switch(pCur->query_type) {
        case QueryType::search: {
          sqlite3_result_double(ctx, pCur->dis->at(pCur->iCurrent));
          break;
        }
        case QueryType::range_search: {
          sqlite3_result_double(ctx, pCur->range_search_result->distances[pCur->iCurrent]);
          break;
        }
      }
      break;
    }
      
  }
  return SQLITE_OK;
}


static int faissIndexBegin(sqlite3_vtab *tab) {
  //printf("BEGIN\n");
  return SQLITE_OK;
}

static int faissIndexSync(sqlite3_vtab *tab) {
  //printf("SYNC\n");
  return SQLITE_OK;
}

static int faissIndexCommit(sqlite3_vtab *pVTab) {
  faissIndex_vtab *p = (faissIndex_vtab*)pVTab;
  printf("COMMIT %d %d\n", p->isTraining, p->isInsertData);
  if(p->isTraining) {
    printf("TRAINING %lu\n", p->training->size());
    p->index->train(p->training->size() / p->dimensions, p->training->data());
    p->isTraining = false;
  }
  if(p->isInsertData) {
    printf("WRITING INDEX\n");
    p->isInsertData = false;
    return write_index_insert(p->index, p->db, p->name);
  }
  return SQLITE_OK;
}

static int faissIndexRollback(sqlite3_vtab *tab) {
  //printf("ROLLBACK\n");
  return SQLITE_OK;
}

static int faissIndexUpdate(
  sqlite3_vtab *pVTab,
  int argc,
  sqlite3_value **argv,
  sqlite_int64 *pRowid
) {
  faissIndex_vtab *p = (faissIndex_vtab*)pVTab;
  if (argc ==1 && sqlite3_value_type(argv[0]) != SQLITE_NULL) {
    printf("xUpdate DELETE \n");
  }
  else if (argc > 1 && sqlite3_value_type(argv[0])== SQLITE_NULL) {
    // if no operation, we adding it to the index
    bool noOperation = sqlite3_value_type(argv[2+FAISSINDEX_COLUMN_OPERATION]) == SQLITE_NULL;
    if (noOperation) {
      std::vector<float>* vec;
      sqlite_int64 rowid = sqlite3_value_int64(argv[1]);
      if ( (vec = valueAsVector(argv[2+FAISSINDEX_COLUMN_VECTOR])) != NULL ) {
        p->index->add_with_ids(1, vec->data(), &rowid);
        p->isInsertData = true;
      }
      
    } else {
      std::string operation ((char *) sqlite3_value_text(argv[2+FAISSINDEX_COLUMN_OPERATION]));
      if(operation.compare("training") == 0) {
        std::vector<float>* vec;
        if ( (vec = valueAsVector(argv[2+FAISSINDEX_COLUMN_VECTOR])) != NULL ) {
          p->training->reserve(vec->size() + distance(vec->begin(), vec->end()));
          p->training->insert(p->training->end(), vec->begin(),vec->end());
          p->isTraining = true;
        }

        //VecX* v = (VecX*) sqlite3_value_pointer(argv[2+FAISSINDEX_COLUMN_VECTOR], "vecx0");
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


static void faissSearchFunc(
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
static void faissRangeSearchFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  printf("range search?\n");
}

static int faissIndexFindFunction(
  sqlite3_vtab *pVtab,
  int nArg,
  const char *zName,
  void (**pxFunc)(sqlite3_context*,int,sqlite3_value**),
  void **ppArg
) {
  //printf("find function. %d %s %s \n", nArg, zName, (char *) sqlite3_version);
  if( sqlite3_stricmp(zName, "faiss_search")==0 ){
    *pxFunc = faissSearchFunc;
    *ppArg = 0;
    return FAISS_SEARCH_FUNCTION;
  }
  if( sqlite3_stricmp(zName, "faiss_range_search")==0 ){
    *pxFunc = faissRangeSearchFunc;
    *ppArg = 0;
    return FAISS_RANGE_SEARCH_FUNCTION;
  }
  return 0;
};

static int faissIndexShadowName(const char *zName){
  static const char *azName[] = {
    "index"
  };
  unsigned int i;
  for(i=0; i<sizeof(azName)/sizeof(azName[0]); i++){
    if( sqlite3_stricmp(zName, azName[i])==0 ) return 1;
  }
  return 0;
}

static sqlite3_module faissIndexModule = {
  /* iVersion    */ 3,
  /* xCreate     */ faissIndexCreate,
  /* xConnect    */ faissIndexConnect,
  /* xBestIndex  */ faissIndexBestIndex,
  /* xDisconnect */ faissIndexDisconnect,
  /* xDestroy    */ faissIndexDestroy,
  /* xOpen       */ faissIndexOpen,
  /* xClose      */ faissIndexClose,
  /* xFilter     */ faissIndexFilter,
  /* xNext       */ faissIndexNext,
  /* xEof        */ faissIndexEof,
  /* xColumn     */ faissIndexColumn,
  /* xRowid      */ faissIndexRowid,
  /* xUpdate     */ faissIndexUpdate,
  /* xBegin      */ faissIndexBegin,
  /* xSync       */ faissIndexSync,
  /* xCommit     */ faissIndexCommit,
  /* xRollback   */ faissIndexRollback,
  /* xFindMethod */ faissIndexFindFunction,
  /* xRename     */ 0,
  /* xSavepoint  */ 0,
  /* xRelease    */ 0,
  /* xRollbackTo */ 0,
  /* xShadowName */ faissIndexShadowName
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
    sqlite3_create_function_v2(db, "faiss_memory_usage", 0, 0, 0, faissMemoryUsageFunc, 0, 0, 0);
    sqlite3_create_function_v2(db, "faiss_vector_debug", 1, 0, 0, faiss_vector_debug, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector", -1, 0, 0, vector, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector_print", 1, 0, 0, vector_print, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector_to_blob", 1, 0, 0, vector_to_blob, 0, 0, 0);
    sqlite3_create_function_v2(db, "faiss_search", 2, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, faissSearchFunc, 0, 0, 0);
    sqlite3_create_function_v2(db, "faiss_search_params", 2, 0, 0, faissSearchParamsFunc, 0, 0, 0);
    sqlite3_create_function_v2(db, "faiss_range_search", 2, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS, 0, faissRangeSearchFunc, 0, 0, 0);
    sqlite3_create_function_v2(db, "faiss_range_search_params", 2, 0, 0, faissRangeSearchParamsFunc, 0, 0, 0);
    
    
    sqlite3_create_module_v2  (db, "faiss_index", &faissIndexModule, 0, 0);
    return 0;
  }
}

#pragma endregion