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
#include <faiss/impl/io.h>




#pragma region work 


struct VecX {
  int64_t size;
  float * data;
};

void del(void*p) {
  delete p;
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


typedef struct templatevtab_vtab templatevtab_vtab;
struct templatevtab_vtab {
  sqlite3_vtab base;  /* Base class - must be first */
  sqlite3 * db;
  char * name;
  faiss::Index * index;
  int dimensions;
  std::vector<float> * training;
  bool isTraining;
  bool isInsertData;
};


typedef struct templatevtab_cursor templatevtab_cursor;
struct templatevtab_cursor {
  sqlite3_vtab_cursor base;  /* Base class - must be first */
  templatevtab_vtab * table;

  sqlite3_int64 iCurrent;
  sqlite3_int64 iRowid;
  std::vector<faiss::idx_t> *nns;
  std::vector<float> *dis;
};

static int init(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr,
  bool isCreate
) {
  int rc;
  rc = sqlite3_declare_vtab(db,"CREATE TABLE x(distance, operation hidden, vector hidden)");
  #define TEMPLATEVTAB_COLUMN_DISTANCE  0
  #define TEMPLATEVTAB_COLUMN_OPERATION  1
  #define TEMPLATEVTAB_COLUMN_VECTOR  2

  if( rc!=SQLITE_OK ) return 0;
  
  templatevtab_vtab *pNew;
  pNew = (templatevtab_vtab *) sqlite3_malloc( sizeof(*pNew) );
  *ppVtab = (sqlite3_vtab*)pNew;
  memset(pNew, 0, sizeof(*pNew));

  if (isCreate) {
    create_shadow_tables(db, argv[1], argv[2]);
    
  } else {
    pNew->index = read_index_select(db, argv[2]);
  }
  int dimensions = std::stoi(argv[3]);
  if (isCreate) {
    //xsize_t n_db = 200;
    //xsize_t n_training = 20000;
    //xfaiss::IndexFlatL2* coarse_quantizer;
    //xcoarse_quantizer = new faiss::IndexFlatL2 (dimensions);
    //xint ncentroids = int(sqrt(n_training));
    //xfaiss::IndexIVFPQ *index = new faiss::IndexIVFPQ(coarse_quantizer, dimensions, ncentroids, 4, 8);
    //IVFx,PQ"y"x"8
    
    char * description = sqlite3_mprintf("%.*s", strlen(argv[4])-2, argv[4]+1);//(char * ) argv[4]+1;
    printf("d=%d, desc=|%s|\n", dimensions, description);
    faiss::Index *index = faiss::index_factory(dimensions, description);
    
    if(!pNew->index)
      pNew->index = index;
    
  }
  pNew->training = new std::vector<float>();
  pNew->isTraining = false;
  pNew->isInsertData = false;
  pNew->name = sqlite3_mprintf("%s", argv[2]);
  pNew->dimensions = dimensions;
  pNew->db = db;
    
  return SQLITE_OK;
}

static int templatevtabCreate(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  return init(db, pAux, argc, argv, ppVtab, pzErr, true);
}

static int templatevtabConnect(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  //printf("connect\n");
  return init(db, pAux, argc, argv, ppVtab, pzErr, false);
}

static int templatevtabDisconnect(sqlite3_vtab *pVtab){
  templatevtab_vtab *p = (templatevtab_vtab*)pVtab;
  sqlite3_free(p);
  return SQLITE_OK;
}

static int templatevtabDestroy(sqlite3_vtab *pVtab){
  //templatevtab_vtab *p = (templatevtab_vtab*)pVtab;
  //sqlite3_free(p);
  return SQLITE_OK;
}

static int templatevtabOpen(sqlite3_vtab *pVtab, sqlite3_vtab_cursor **ppCursor){
  templatevtab_cursor *pCur;
  templatevtab_vtab *p = (templatevtab_vtab*)pVtab;
  pCur = (templatevtab_cursor *) sqlite3_malloc( sizeof(*pCur) );
  if( pCur==0 ) return SQLITE_NOMEM;
  memset(pCur, 0, sizeof(*pCur));
  *ppCursor = &pCur->base;
  pCur->table = p;
  pCur->nns = new std::vector<faiss::idx_t>(5);
  pCur->dis = new std::vector<float>(5);


  
  return SQLITE_OK;
}

static int templatevtabClose(sqlite3_vtab_cursor *cur){
  templatevtab_cursor *pCur = (templatevtab_cursor*)cur;
  sqlite3_free(pCur);
  return SQLITE_OK;
}

static int templatevtabColumn(
  sqlite3_vtab_cursor *cur,
  sqlite3_context *ctx,
  int i
){
  templatevtab_cursor *pCur = (templatevtab_cursor*)cur;
  switch( i ){
    case TEMPLATEVTAB_COLUMN_DISTANCE: 
      sqlite3_result_double(ctx, pCur->dis->at(pCur->iCurrent));
  }
  return SQLITE_OK;
}


static int templatevtabBestIndex(
  sqlite3_vtab *tab,
  sqlite3_index_info *pIdxInfo
){
  pIdxInfo->estimatedCost = (double)10;
  pIdxInfo->estimatedRows = 10;

  for(int i = 0; i < pIdxInfo->nConstraint; i++) {
    if(pIdxInfo->aConstraint[i].usable && pIdxInfo->aConstraint[i].iColumn == TEMPLATEVTAB_COLUMN_VECTOR) {
      pIdxInfo->aConstraintUsage[i].argvIndex = 1;
      pIdxInfo->aConstraintUsage[i].omit = 1;
      pIdxInfo->estimatedCost = (double)5;
    }
  }
  return SQLITE_OK;
}

static int templatevtabFilter(
  sqlite3_vtab_cursor *pVtabCursor, 
  int idxNum, const char *idxStr,
  int argc, sqlite3_value **argv
){
  templatevtab_cursor *pCur = (templatevtab_cursor *)pVtabCursor;
  VecX* v = (VecX*) sqlite3_value_pointer(argv[0], "vecx0");
  std::vector<float> vvv(v->data, v->data + v->size);

  int nq = 1;
  int k = 5;
  std::vector<faiss::idx_t> nns(k * nq);
  std::vector<float> dis(k * nq);
  pCur->table->index->search(nq, vvv.data(), k, pCur->dis->data(), pCur->nns->data());
  pCur->iCurrent = 0;
  return SQLITE_OK;
}

static int templatevtabNext(sqlite3_vtab_cursor *cur){
  templatevtab_cursor *pCur = (templatevtab_cursor*)cur;
  pCur->iCurrent++;
  return SQLITE_OK;
}
static int templatevtabRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid){
  templatevtab_cursor *pCur = (templatevtab_cursor*)cur;
  *pRowid = pCur->nns->at(pCur->iCurrent);
  return SQLITE_OK;
}

static int templatevtabEof(sqlite3_vtab_cursor *cur){
  templatevtab_cursor *pCur = (templatevtab_cursor*)cur;
  return pCur->iCurrent >= 5;
}


static int templatevtabBegin(sqlite3_vtab *tab) {
  printf("BEGIN\n");
  return SQLITE_OK;
}

static int templatevtabSync(sqlite3_vtab *tab) {
  printf("SYNC\n");
  return SQLITE_OK;
}

static int templatevtabCommit(sqlite3_vtab *pVTab) {
  printf("COMMIT\n");
  templatevtab_vtab *p = (templatevtab_vtab*)pVTab;
  if(p->isTraining) {
    printf("TRAINING %d\n", p->training->size());
    p->index->train(p->training->size() / p->dimensions, p->training->data());
    p->isTraining = false;
  }
  if(p->isInsertData) {
    printf("WRITING INDEX\n");
    p->isTraining = false;
    return write_index_insert(p->index, p->db, p->name);
  }
  return SQLITE_OK;
}

static int templatevtabRollback(sqlite3_vtab *tab) {
  printf("ROLLBACK\n");
  return SQLITE_OK;
}

// https://github.com/sqlite/sqlite/blob/master/src/json.c#L88-L89
#define JSON_SUBTYPE  74    /* Ascii for "J" */

static std::vector<float>* valueAsVector(sqlite3_value*value) {
    VecX* v = (VecX*) sqlite3_value_pointer(value, "vecx0");
    if (v!=NULL) return new std::vector<float>(v->data, v->data + v->size);

    if(sqlite3_value_subtype(value) == JSON_SUBTYPE) {

    }
    return NULL;
    
}

static int templatevtabUpdate(
  sqlite3_vtab *pVTab,
  int argc,
  sqlite3_value **argv,
  sqlite_int64 *pRowid
) {
  templatevtab_vtab *p = (templatevtab_vtab*)pVTab;
  if (argc ==1 && sqlite3_value_type(argv[0]) != SQLITE_NULL) {
    printf("xUpdate DELETE \n");
  }
  else if (argc > 1 && sqlite3_value_type(argv[0])== SQLITE_NULL) {
    // if no operation, we adding it to the index
    bool noOperation = sqlite3_value_type(argv[2+TEMPLATEVTAB_COLUMN_OPERATION]) == SQLITE_NULL;
    if (noOperation) {
      std::vector<float>* vec;
      sqlite_int64 rowid = sqlite3_value_int64(argv[1]);
      if ( (vec = valueAsVector(argv[2+TEMPLATEVTAB_COLUMN_VECTOR])) != NULL ) {
        p->index->add_with_ids(1, vec->data(), &rowid);
        p->isInsertData = true;
      }
      
    } else {
      std::string operation ((char *) sqlite3_value_text(argv[2+TEMPLATEVTAB_COLUMN_OPERATION]));
      if(operation.compare("training") == 0) {
        std::vector<float>* vec;
        if ( (vec = valueAsVector(argv[2+TEMPLATEVTAB_COLUMN_VECTOR])) != NULL ) {
          p->training->reserve(vec->size() + distance(vec->begin(), vec->end()));
          p->training->insert(p->training->end(), vec->begin(),vec->end());
          p->isTraining = true;
        }

        //VecX* v = (VecX*) sqlite3_value_pointer(argv[2+TEMPLATEVTAB_COLUMN_VECTOR], "vecx0");
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


static int templatevtabShadowName(const char *zName){
  static const char *azName[] = {
    "index"
  };
  unsigned int i;
  for(i=0; i<sizeof(azName)/sizeof(azName[0]); i++){
    if( sqlite3_stricmp(zName, azName[i])==0 ) return 1;
  }
  return 0;
}

static sqlite3_module templatevtabModule = {
  /* iVersion    */ 3,
  /* xCreate     */ templatevtabCreate,
  /* xConnect    */ templatevtabConnect,
  /* xBestIndex  */ templatevtabBestIndex,
  /* xDisconnect */ templatevtabDisconnect,
  /* xDestroy    */ templatevtabDestroy,
  /* xOpen       */ templatevtabOpen,
  /* xClose      */ templatevtabClose,
  /* xFilter     */ templatevtabFilter,
  /* xNext       */ templatevtabNext,
  /* xEof        */ templatevtabEof,
  /* xColumn     */ templatevtabColumn,
  /* xRowid      */ templatevtabRowid,
  /* xUpdate     */ templatevtabUpdate,
  /* xBegin      */ templatevtabBegin,
  /* xSync       */ templatevtabSync,
  /* xCommit     */ templatevtabCommit,
  /* xRollback   */ templatevtabRollback,
  /* xFindMethod */ 0,
  /* xRename     */ 0,
  /* xSavepoint  */ 0,
  /* xRelease    */ 0,
  /* xRollbackTo */ 0,
  /* xShadowName */ templatevtabShadowName
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
    sqlite3_create_function_v2(db, "faiss_vector_debug", 1, 0, 0, faiss_vector_debug, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector", -1, 0, 0, vector, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector_print", 1, 0, 0, vector_print, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector_to_blob", 1, 0, 0, vector_to_blob, 0, 0, 0);
    
    sqlite3_create_module_v2  (db, "templatevtab", &templatevtabModule, 0, 0);
    return 0;
  }
}

#pragma endregion