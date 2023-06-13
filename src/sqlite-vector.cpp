#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "sqlite-vector.h"
#include "sqlite-vss.h"

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

using namespace std;

typedef unique_ptr<vector<float>> vec_ptr;

// https://github.com/sqlite/sqlite/blob/master/src/json.c#L88-L89
#define JSON_SUBTYPE  74    /* Ascii for "J" */

#include <nlohmann/json.hpp>
using json = nlohmann::json;

char VECTOR_BLOB_HEADER_BYTE = 'v';
char VECTOR_BLOB_HEADER_TYPE = 1;
const char * VECTOR_FLOAT_POINTER_NAME = "vectorf32v0";

struct VecX {
  int64_t size;
  float * data;
};

void delVectorFloat(void * p) {
  VectorFloat * vx = (VectorFloat *)p;
  sqlite3_free(vx->data);
  delete vx;
}

struct Vector0Global {
  vector0_api api;
  sqlite3 *db;
};

void resultVector(sqlite3_context * context, vector<float> * vecIn) {

  VectorFloat * vecRes = new VectorFloat();
  vecRes->size = vecIn->size();
  vecRes->data = (float *) sqlite3_malloc(vecIn->size() * sizeof(float));
  memcpy(vecRes->data, vecIn->data(), vecIn->size() * sizeof(float));
  sqlite3_result_pointer(context, vecRes, VECTOR_FLOAT_POINTER_NAME, nullptr);
}

#pragma region generic

vec_ptr vectorFromBlobValue(sqlite3_value * value, const char ** pzErrMsg) {
  int n = sqlite3_value_bytes(value);
  const void * b;
  char header;
  char type;

  if(n < (2)) {
    *pzErrMsg = "Vector blob size less than header length";
    return NULL;
  }
  b = sqlite3_value_blob(value);
  memcpy(&header, ((char *) b + 0), sizeof(char));
  memcpy(&type,   ((char *) b + 1), sizeof(char));

  if(header != VECTOR_BLOB_HEADER_BYTE) {
    *pzErrMsg = "Blob not well-formatted vector blob";
    return NULL;
  }

  if(type != VECTOR_BLOB_HEADER_TYPE) {
    *pzErrMsg = "Blob type not right";
    return NULL;
  }

  int numElements = (n - 2)/sizeof(float);
  float * v = (float *) ((char *)b + 2);
  return vec_ptr(new vector<float>(v, v + numElements));
}

vec_ptr vectorFromRawBlobValue(sqlite3_value * value, const char ** pzErrMsg) {
  int n = sqlite3_value_bytes(value);
  // must be divisible by 4
  if(n%4) {
    *pzErrMsg = "Invalid raw blob length, must be divisible by 4";
    return NULL;
  }
  const void * b = sqlite3_value_blob(value);

  float * v = (float *) ((char *)b);
  return vec_ptr(new vector<float>(v, v + (n / 4)));
}

vec_ptr vectorFromTextValue(sqlite3_value*value) {
  vec_ptr v;

  try {
    json data = json::parse(sqlite3_value_text(value));
    data.get_to(*v);
  } catch (const json::exception&) {
    return NULL;
  }

  return v;
}

// Returns vector pointer MUST be deleted
static vec_ptr valueAsVector(sqlite3_value * value) {

  // Option 1: If the value is a "vectorf32v0" pointer, create vector from that
  VectorFloat* v = (VectorFloat*) sqlite3_value_pointer(value, VECTOR_FLOAT_POINTER_NAME);
  if (v!=NULL) return vec_ptr(new vector<float>(v->data, v->data + v->size));
  vec_ptr vec;

  // Option 2: value is a blob in vector format
  if(sqlite3_value_type(value) == SQLITE_BLOB) {
    const char * pzErrMsg = 0;
    if((vec = vectorFromBlobValue(value, &pzErrMsg)) != NULL) {
      return vec;
    }
    if((vec = vectorFromRawBlobValue(value, &pzErrMsg)) != NULL) {
      return vec;
    }
  }
  // Option 3: if value is a JSON array coercible to float vector, use that
  //if(sqlite3_value_subtype(value) == JSON_SUBTYPE) {
  if(sqlite3_value_type(value) == SQLITE_TEXT) {
    if((vec = vectorFromTextValue(value)) != NULL) {
      return vec;
    }else {
      return NULL;
    }
  }

  // else, value isn't a vector
  return NULL;
}
#pragma endregion

#pragma region meta

static void vector_version(sqlite3_context * context, int argc, sqlite3_value ** argv) {
  sqlite3_result_text(context, SQLITE_VSS_VERSION, -1, SQLITE_STATIC);
}

static void vector_debug(sqlite3_context *context, int argc, sqlite3_value **argv) {
  if(argc){
    vec_ptr v = valueAsVector(argv[0]);
    if(v==NULL) {
      sqlite3_result_error(context, "value not a vector", -1);
      return;
    }
    sqlite3_str * str = sqlite3_str_new(0);
    sqlite3_str_appendf(str, "size: %lld [", v->size());
    for(int i = 0; i < v->size(); i++) {
      if(i==0) sqlite3_str_appendf(str, "%f", v->at(i));
      else sqlite3_str_appendf(str, ", %f", v->at(i));
    }
    sqlite3_str_appendchar(str, 1, ']');
    sqlite3_result_text(context, sqlite3_str_finish(str), -1, sqlite3_free);

  }else {
    sqlite3_result_text(context, "yo", -1, SQLITE_STATIC);
  }
}
#pragma endregion


#pragma region vector generation

// TODO should return fvec, ivec, or bvec depending on input. How do bvec, though?
static void vector_from(sqlite3_context * context, int argc, sqlite3_value **argv) {

  vector<float> vec(argc);
  for(int i = 0; i < argc; i++) {
    vec.push_back(sqlite3_value_double(argv[i]));
  }
  resultVector(context, &vec);
}

#pragma endregion

#pragma region vector general

static void vector_value_at(sqlite3_context *context, int argc, sqlite3_value **argv) {
  vec_ptr v = valueAsVector(argv[0]);
  if(v == NULL) return;
  int at = sqlite3_value_int(argv[1]);
  try {
    float result = v->at(at);
    sqlite3_result_double(context, result);
  }
   catch (const out_of_range& oor) {
    char * errmsg = sqlite3_mprintf("%d out of range: %s", at, oor.what());
    if(errmsg != NULL){
      sqlite3_result_error(context, errmsg, -1);
      sqlite3_free(errmsg);
    }
    else sqlite3_result_error_nomem(context);
  }
}

static void vector_length(sqlite3_context *context, int argc, sqlite3_value **argv) {
  VectorFloat * v = (VectorFloat*) sqlite3_value_pointer(argv[0], VECTOR_FLOAT_POINTER_NAME);
  if(v==NULL) return;
  sqlite3_result_int64(context, v->size);
}

#pragma endregion

#pragma region json

static void vector_to_json(sqlite3_context *context, int argc, sqlite3_value **argv) {

  vec_ptr v = valueAsVector(argv[0]);
  if(v == NULL) return;

  json j = json(*v);
  sqlite3_result_text(context, j.dump().c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_result_subtype(context, JSON_SUBTYPE);
}

static void vector_from_json(sqlite3_context *context, int argc, sqlite3_value **argv) {

  const char * text = (const char *) sqlite3_value_text(argv[0]);
  vec_ptr v = vectorFromTextValue(argv[0]);
  if(v == NULL) {
    sqlite3_result_error(context, "input not valid json, or contains non-float data", -1);
  }else {
    resultVector(context, v.get());
  }
}
#pragma endregion

#pragma region blob

/*

|Offset | Size | Description
|-|-|-
|a|a|A
*/
static void vector_to_blob(sqlite3_context *context, int argc, sqlite3_value **argv) {
  vec_ptr v = valueAsVector(argv[0]);
  if(v == NULL) return;

  int sz = v->size();
  int n = (sizeof(char)) + (sizeof(char)) + (sz * 4);
  void * b = sqlite3_malloc(n);
  memset(b, 0, n);

  memcpy((void *) ((char *) b+0), (void *) &VECTOR_BLOB_HEADER_BYTE, sizeof(char));
  memcpy((void *) ((char *) b+1), (void *) &VECTOR_BLOB_HEADER_TYPE, sizeof(char));
  memcpy((void *) ((char *) b+2), (void *) v->data(), sz*4);
  sqlite3_result_blob64(context, b, n, sqlite3_free);

}

static void vector_from_blob(sqlite3_context *context, int argc, sqlite3_value **argv) {
  const char * pzErrMsg;
  vec_ptr vec = vectorFromBlobValue(argv[0], &pzErrMsg);
  if(vec == NULL) {
    sqlite3_result_error(context, pzErrMsg, -1);
  } else {
    resultVector(context, vec.get());
  }
}

static void vector_to_raw(sqlite3_context *context, int argc, sqlite3_value **argv) {
  vec_ptr v = valueAsVector(argv[0]);
  if(v == NULL) return;

  int sz = v->size();
  int n = sz * sizeof(float);
  void * b = sqlite3_malloc(n);
  memset(b, 0, n);
  memcpy((void *) ((char *) b), (void *) v->data(), n);
  sqlite3_result_blob64(context, b, n, sqlite3_free);
}

static void vector_from_raw(sqlite3_context *context, int argc, sqlite3_value **argv) {
  const char * pzErrMsg;
  vec_ptr vec = vectorFromRawBlobValue(argv[0], &pzErrMsg);
  if(vec == NULL) {
    sqlite3_result_error(context, pzErrMsg, -1);
  } else {
    resultVector(context, vec.get());
  }
}

#pragma endregion

#pragma region fvecs vtab

typedef struct fvecsEach_vtab fvecsEach_vtab;

struct fvecsEach_vtab {
  sqlite3_vtab base;  /* Base class - must be first */
};

struct fvecsEach_cursor {

  sqlite3_vtab_cursor base;  /* Base class - must be first */
  sqlite3_int64 iRowid;

  // malloc'ed copy of fvecs input blob
  void * pBlob;

  // total size of pBlob in bytes
  sqlite3_int64 iBlobN;
  sqlite3_int64 p;

  // current dimensions
  int iCurrentD;

  // pointer to current vector being read in
  vec_ptr pCurrentVector;
};

static int fvecsEachConnect(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr) {

  fvecsEach_vtab *pNew;
  int rc;

  rc = sqlite3_declare_vtab(db,
           "CREATE TABLE x(dimensions, vector, input hidden)"
       );
#define FVECS_EACH_DIMENSIONS   0
#define FVECS_EACH_VECTOR       1
#define FVECS_EACH_INPUT        2
  if( rc==SQLITE_OK ){
    pNew = (fvecsEach_vtab *) sqlite3_malloc( sizeof(*pNew) );
    *ppVtab = (sqlite3_vtab*)pNew;
    if( pNew==0 ) return SQLITE_NOMEM;
    memset(pNew, 0, sizeof(*pNew));
  }
  return rc;
}

static int fvecsEachDisconnect(sqlite3_vtab *pVtab) {

  fvecsEach_vtab * p = (fvecsEach_vtab*)pVtab;
  sqlite3_free(p);
  return SQLITE_OK;
}

static int fvecsEachOpen(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor) {

  fvecsEach_cursor *pCur;
  pCur = (fvecsEach_cursor *)sqlite3_malloc( sizeof(*pCur) );
  if( pCur==0 ) return SQLITE_NOMEM;
  memset(pCur, 0, sizeof(*pCur));
  *ppCursor = &pCur->base;
  return SQLITE_OK;
}

static int fvecsEachClose(sqlite3_vtab_cursor *cur){
  fvecsEach_cursor *pCur = (fvecsEach_cursor*)cur;
  sqlite3_free(pCur);
  return SQLITE_OK;
}

static int fvecsEachBestIndex(
  sqlite3_vtab *tab,
  sqlite3_index_info *pIdxInfo) {

  for (int i = 0; i < pIdxInfo->nConstraint; i++) {
    auto pCons = pIdxInfo->aConstraint[i];
    switch (pCons.iColumn) {
      case FVECS_EACH_INPUT: {
        if (pCons.op == SQLITE_INDEX_CONSTRAINT_EQ && pCons.usable) {
          pIdxInfo->aConstraintUsage[i].argvIndex = 1;
          pIdxInfo->aConstraintUsage[i].omit = 1;
        }
        break;
      }
    }
    }
  pIdxInfo->estimatedCost = (double)10;
  pIdxInfo->estimatedRows = 10;
  return SQLITE_OK;
}

static int fvecsEachFilter(
  sqlite3_vtab_cursor *pVtabCursor,
  int idxNum, const char *idxStr,
  int argc, sqlite3_value **argv
){
  fvecsEach_cursor *pCur = (fvecsEach_cursor *)pVtabCursor;

  int n = sqlite3_value_bytes(argv[0]);
  const void * b = sqlite3_value_blob(argv[0]);

  pCur->pBlob = sqlite3_malloc(n);
  pCur->iBlobN = n;
  pCur->iRowid = 1;
  memcpy(pCur->pBlob, b, n);

  memcpy(&pCur->iCurrentD, pCur->pBlob, sizeof(int));
  float * v = (float *) ((char *)pCur->pBlob + sizeof(int));
  pCur->pCurrentVector = vec_ptr(new vector<float>(v, v+pCur->iCurrentD));
  pCur->p = sizeof(int) + (pCur->iCurrentD*sizeof(float));

  return SQLITE_OK;
}

static int fvecsEachNext(sqlite3_vtab_cursor *cur){
  fvecsEach_cursor *pCur = (fvecsEach_cursor*)cur;

  memcpy(&pCur->iCurrentD, ((char *)pCur->pBlob + pCur->p), sizeof(int));
  float * v = (float *) (((char *)pCur->pBlob + pCur->p) + sizeof(int));
  pCur->pCurrentVector->clear();
  pCur->pCurrentVector->reserve(pCur->iCurrentD);// = new vector<float>(v, v+pCur->iCurrentD);
  pCur->pCurrentVector->insert(pCur->pCurrentVector->begin(), v, v+pCur->iCurrentD);

  pCur->p += (sizeof(int) + (pCur->iCurrentD*sizeof(float)));
  pCur->iRowid++;
  return SQLITE_OK;
}

static int fvecsEachEof(sqlite3_vtab_cursor *cur){
  fvecsEach_cursor *pCur = (fvecsEach_cursor*)cur;

  return pCur->p > pCur->iBlobN;
}

static int fvecsEachRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid){
  fvecsEach_cursor *pCur = (fvecsEach_cursor*)cur;
  *pRowid = pCur->iRowid;
  return SQLITE_OK;
}

static int fvecsEachColumn(
  sqlite3_vtab_cursor *cur,   /* The cursor */
  sqlite3_context *context,       /* First argument to sqlite3_result_...() */
  int i                       /* Which column to return */
){
  fvecsEach_cursor *pCur = (fvecsEach_cursor*)cur;
  switch( i ){
    case FVECS_EACH_DIMENSIONS:
      sqlite3_result_int(context, pCur->iCurrentD);
      break;
    case FVECS_EACH_VECTOR:
      resultVector(context, pCur->pCurrentVector.get());
      break;
    case FVECS_EACH_INPUT:
      sqlite3_result_null(context);
      break;
  }
  return SQLITE_OK;
}

/*
** This following structure defines all the methods for the
** virtual table.
*/
static sqlite3_module fvecsEachModule = {
  /* iVersion    */ 0,
  /* xCreate     */ fvecsEachConnect,
  /* xConnect    */ fvecsEachConnect,
  /* xBestIndex  */ fvecsEachBestIndex,
  /* xDisconnect */ fvecsEachDisconnect,
  /* xDestroy    */ 0,
  /* xOpen       */ fvecsEachOpen,
  /* xClose      */ fvecsEachClose,
  /* xFilter     */ fvecsEachFilter,
  /* xNext       */ fvecsEachNext,
  /* xEof        */ fvecsEachEof,
  /* xColumn     */ fvecsEachColumn,
  /* xRowid      */ fvecsEachRowid,
  /* xUpdate     */ 0,
  /* xBegin      */ 0,
  /* xSync       */ 0,
  /* xCommit     */ 0,
  /* xRollback   */ 0,
  /* xFindMethod */ 0,
  /* xRename     */ 0,
  /* xSavepoint  */ 0,
  /* xRelease    */ 0,
  /* xRollbackTo */ 0,
  /* xShadowName */ 0
};

#pragma endregion


#pragma region fvecs

static void vector_fvecs(sqlite3_context *context, int argc, sqlite3_value **argv) {
  sqlite3_int64 sz = sqlite3_value_bytes(argv[0]);
  const void * blob = sqlite3_value_blob(argv[0]);
  int d;
  memcpy((void *) &d, (void *) blob, sizeof(int));
  if(d <= 0 || d >= 1000000) {
    sqlite3_result_error(context, "unreasonable dimensions size", -1);
    return;
  }
  if( sz % ((d + 1) * 4) != 0) {
    sqlite3_result_error(context, "wrong blob size", -1);
    return;
  }
  size_t n = sz / ((d + 1) * 4);
  printf("sz=%lld, d=%d n=%zu\n", sz, d, n);

  float* x = new float[n * (d + 1)];
  memcpy(x, ((char*) blob) + sizeof(int), (sizeof(float)) * (n * (d + 1)));
  //for (size_t i = 0; i < n; i++)
  //      memmove(x + i * d, x + 1 + i * (d + 1), d * sizeof(*x));

  printf("x[0]=%f \n", x[0]);
  printf("x[1]=%f \n", x[1]);
  //sqlite3_result_text(context, "yo", -1, SQLITE_STATIC);
}
#pragma endregion


#pragma region entrypoint
static void vector0(sqlite3_context *context, int argc, sqlite3_value **argv) {
  Vector0Global *pGlobal = (Vector0Global*)sqlite3_user_data(context);
  vector0_api **ppApi;
  ppApi = (vector0_api**)sqlite3_value_pointer(argv[0], "vector0_api_ptr");
  if( ppApi ) *ppApi = &pGlobal->api;
}

extern "C" {
  #ifdef _WIN32
  __declspec(dllexport)
  #endif
  int sqlite3_vector_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) {
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);
    Vector0Global *pGlobal = 0;
    pGlobal = (Vector0Global*)sqlite3_malloc(sizeof(Vector0Global));
    if( pGlobal==0 ){
      return SQLITE_NOMEM;
    }
     void *p = (void*)pGlobal;
    memset(pGlobal, 0, sizeof(Vector0Global));
    pGlobal->db = db;
    pGlobal->api.iVersion = 0;
    pGlobal->api.xValueAsVector = valueAsVector;
    pGlobal->api.xResultVector = resultVector;
    rc = sqlite3_create_function_v2(db, "vector0", 1,
                               SQLITE_UTF8,
                               p,
                               vector0, 0, 0, sqlite3_free);
     static const struct {
    char *zFName;
    int nArg;
    void* pAux;
    void (*xFunc)(sqlite3_context*,int,sqlite3_value**);
    int flags;
  } aFunc[] = {
    //{ (char*) "vector0",            1,  p,    vector0,          SQLITE_UTF8 },
    { (char*) "vector_version",     0,  NULL, vector_version,   SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS },
    { (char*) "vector_debug",       0,  NULL, vector_debug,     SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS },
    { (char*) "vector_debug",       1,  NULL, vector_debug,     SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS },
    { (char*) "vector_length",      1,  NULL, vector_length,    SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS },
    { (char*) "vector_value_at",    2,  NULL, vector_value_at,  SQLITE_UTF8|SQLITE_INNOCUOUS},
    { (char*) "vector_from_json",   1,  NULL, vector_from_json, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS},
    { (char*) "vector_to_json",     1,  NULL, vector_to_json,   SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS},
    { (char*) "vector_from_blob",   1,  NULL, vector_from_blob, SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS},
    { (char*) "vector_to_blob",     1,  NULL, vector_to_blob,   SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS},
    { (char*) "vector_from_raw",    1,  NULL, vector_from_raw,  SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS},
    { (char*) "vector_to_raw",      1,  NULL, vector_to_raw,    SQLITE_UTF8|SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS},
  };
    for(int i=0; i<sizeof(aFunc)/sizeof(aFunc[0]) && rc==SQLITE_OK; i++){
      rc = sqlite3_create_function_v2(db, aFunc[i].zFName, aFunc[i].nArg,
                               aFunc[i].flags,
                               aFunc[i].pAux,
                               aFunc[i].xFunc, 0, 0, 0);
      if(rc != SQLITE_OK) {
        *pzErrMsg = sqlite3_mprintf("%s: %s", aFunc[i].zFName, sqlite3_errmsg(db));
        return rc;
      }
    }


    rc = sqlite3_create_module(db, "vector_fvecs_each", &fvecsEachModule, 0);
    if(rc != SQLITE_OK) goto fail;


    return SQLITE_OK;

    fail:
      *pzErrMsg = sqlite3_mprintf("%s", sqlite3_errmsg(db));
      return rc;
  }
}

#pragma endregion
