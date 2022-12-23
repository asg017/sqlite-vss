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
#include <faiss/index_io.h>




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

struct Doub {
  faiss::IndexIVFPQ * index;
  std::vector<float> * training;
};
static void train(sqlite3_context *context, int argc, sqlite3_value **argv) {
  Doub * doub = (Doub *) sqlite3_user_data(context);
  faiss::IndexIVFPQ * index = doub->index;
  std::vector<float> * training = doub->training;
  index->verbose = true;
  index->train(training->size() / 384, training->data());
  sqlite3_result_int(context, 1);
}

static void add_data(sqlite3_context *context, int argc, sqlite3_value **argv) {
  faiss::IndexIVFPQ * index = (faiss::IndexIVFPQ *) sqlite3_user_data(context);
  VecX* v = (VecX*) sqlite3_value_pointer(argv[0], "vecx0");
  std::vector<float> vvv(v->data, v->data + v->size);
  //index->add_with_ids(100, float{1}, float{2});
  sqlite3_result_int(context, 1);
}

#pragma endregion

#pragma region vtab

typedef struct templatevtab_vtab templatevtab_vtab;
struct templatevtab_vtab {
  sqlite3_vtab base;  /* Base class - must be first */
  faiss::Index * index;
  int dimensions;
  std::vector<float> * training;
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

static int templatevtabConnect(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  templatevtab_vtab *pNew;
  int rc;

  rc = sqlite3_declare_vtab(db,"CREATE TABLE x(distance, operation hidden, vector hidden)");

#define TEMPLATEVTAB_COLUMN_DISTANCE  0
#define TEMPLATEVTAB_COLUMN_OPERATION  1
#define TEMPLATEVTAB_COLUMN_VECTOR  2

  if( rc==SQLITE_OK ){
    pNew = (templatevtab_vtab *) sqlite3_malloc( sizeof(*pNew) );
    *ppVtab = (sqlite3_vtab*)pNew;
    if( pNew==0 ) return SQLITE_NOMEM;
    memset(pNew, 0, sizeof(*pNew));

    int dimensions = 384;

    size_t n_db = 200 * 10;
    size_t n_training = 100 * 10;

    faiss::IndexFlatL2* coarse_quantizer;
    coarse_quantizer = new faiss::IndexFlatL2 (dimensions);
    int ncentroids = int(sqrt(n_training));
    faiss::IndexIVFPQ *index = new faiss::IndexIVFPQ(coarse_quantizer, dimensions, ncentroids, 4, 8);
    

    pNew->index = index;
    pNew->dimensions = dimensions;
    pNew->training = new std::vector<float>();
    
  }
  return rc;
}

static int templatevtabDisconnect(sqlite3_vtab *pVtab){
  templatevtab_vtab *p = (templatevtab_vtab*)pVtab;
  sqlite3_free(p);
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
  //printf("ntotal=%lld, trained=%d, vvv.size()=%lld\n", pCur->table->index->ntotal, pCur->table->index->is_trained, vvv.size());
  pCur->table->index->search(nq, vvv.data(), k, pCur->dis->data(), pCur->nns->data());

  /*for (int i = 0; i < nq; i++) {
      printf("query %2d: ", i);
      for (int j = 0; j < k; j++) {
          printf("%7lld ", nns[j + i * k]);
      }
      printf("\n     dis: ");
      for (int j = 0; j < k; j++) {
          printf("%7g ", dis[j + i * k]);
      }
      printf("\n");
  }
  printf("\n");*/
  pCur->iCurrent = 0;
  //pCur->iRowid = 1;
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
  printf("begin\n");
  return SQLITE_OK;
}


static int templatevtabCommit(sqlite3_vtab *tab) {
  printf("commit\n");
  return SQLITE_OK;
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
    //printf("xUpdate INSERT %d \n", argc);
    /*if(sqlite3_value_type(argv[1]) == SQLITE_NULL) {
      return SQLITE_ERROR;
    }
    sqlite_int64 rowid = sqlite3_value_int64(argv[1]);*/
    
    
    
    // if no operation, we adding it to the index
    if (sqlite3_value_type(argv[2+TEMPLATEVTAB_COLUMN_OPERATION]) == SQLITE_NULL) {
      VecX* v = (VecX*) sqlite3_value_pointer(argv[2+TEMPLATEVTAB_COLUMN_VECTOR], "vecx0");
      sqlite_int64 rowid = sqlite3_value_int64(argv[1]);
      std::vector<float> vvv(v->data, v->data + v->size);
      p->index->add_with_ids(1, vvv.data(), &rowid);
    } else {
      std::string operation ((char *) sqlite3_value_text(argv[2+TEMPLATEVTAB_COLUMN_OPERATION]));
      if(operation.compare("training") == 0) {
        VecX* v = (VecX*) sqlite3_value_pointer(argv[2+TEMPLATEVTAB_COLUMN_VECTOR], "vecx0");
        std::vector<float> vvv(v->data, v->data + v->size);
        p->training->reserve(vvv.size() + distance(vvv.begin(), vvv.end()));
        p->training->insert(p->training->end(), vvv.begin(),vvv.end());
      
      }else if (operation.compare("train") == 0) {
        //printf("train size=%ld n=%ld\n", p->training->size(), p->training->size() / p->dimensions);
        //p->index->verbose = true;
        p->index->train(p->training->size() / p->dimensions, p->training->data());
        //printf("done training?\n");

      } 
      else if (operation.compare("debug") == 0) {
        printf("debugging!\n");
        FILE *stream;
        char *buf;
        size_t len;
        stream = open_memstream (&buf, &len);
        write_index(p->index, stream);
        printf("length=%d\n", len);
        //printf("done training?\n");

      } else {
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


static sqlite3_module templatevtabModule = {
  /* iVersion    */ 0,
  /* xCreate     */ 0,
  /* xConnect    */ templatevtabConnect,
  /* xBestIndex  */ templatevtabBestIndex,
  /* xDisconnect */ templatevtabDisconnect,
  /* xDestroy    */ 0,
  /* xOpen       */ templatevtabOpen,
  /* xClose      */ templatevtabClose,
  /* xFilter     */ templatevtabFilter,
  /* xNext       */ templatevtabNext,
  /* xEof        */ templatevtabEof,
  /* xColumn     */ templatevtabColumn,
  /* xRowid      */ templatevtabRowid,
  /* xUpdate     */ templatevtabUpdate,
  /* xBegin      */ templatevtabBegin,
  /* xSync       */ 0,
  /* xCommit     */ templatevtabCommit,
  /* xRollback   */ 0,
  /* xFindMethod */ 0,
  /* xRename     */ 0,
  /* xSavepoint  */ 0,
  /* xRelease    */ 0,
  /* xRollbackTo */ 0,
  /* xShadowName */ 0
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

    // dimension of the vectors to index
    int d = 384;

    // size of the database we plan to index
    size_t nb = 200000;

    // make a set of nt training vectors in the unit cube
    // (could be the database)
    size_t nt = 20000;

    // make the index object and train it
    faiss::IndexFlatL2* coarse_quantizer;//(d);
    coarse_quantizer = new faiss::IndexFlatL2 (d);

    // a reasonable number of centroids to index nb vectors
    int ncentroids = int(sqrt(nt));

    // the coarse quantizer should not be dealloced before the index
    // 4 = nb of bytes per code (d must be a multiple of this)
    // 8 = nb of bits per sub-code (almost always 8)
    faiss::IndexIVFPQ *index;//(&coarse_quantizer, d, ncentroids, 4, 8);
    index = new faiss::IndexIVFPQ(coarse_quantizer, d, ncentroids, 4, 8);

    /*std::mt19937 rng;

    { // training
        std::vector<float> trainvecs(nt * d);
        std::uniform_real_distribution<> distrib;
        for (size_t i = 0; i < nt * d; i++) {
            trainvecs[i] = distrib(rng);
        }

        //index->verbose = true;
        index->train(nt, trainvecs.data());
    }*/

    /*{ // I/O demo
        const char* outfilename = "./index_trained.faissindex";
        printf("[%.3f s] storing the pre-trained index to %s\n",
               elapsed() - t0,
               outfilename);

        printf("xx=%zu\n", index->sa_code_size());
        
        FILE *stream;
        char *buf;
        size_t len;
        stream = open_memstream (&buf, &len);
        write_index(index, stream);
        printf("len=%d\n", len);
    }*/

    
    /*{ // populating the database
        std::vector<float> database(nb * d);
        std::uniform_real_distribution<> distrib;
        for (size_t i = 0; i < nb * d; i++) {
            database[i] = distrib(rng);
        }

        index->add(nb, database.data());

        FILE *stream;
        char *buf;
        size_t len;
        stream = open_memstream (&buf, &len);
        write_index(index, stream);
        printf("len=%d\n", len);

        fclose(stream);
    }*/

    std::vector<float> *trainingx = new std::vector<float>;

    Doub * doub = new Doub();
    doub->index = index;
    doub->training = trainingx;
    sqlite3_create_function_v2(db, "faiss_vector_debug", 1, 0, 0, faiss_vector_debug, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector", -1, 0, 0, vector, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector_print", 1, 0, 0, vector_print, 0, 0, 0);
    sqlite3_create_function_v2(db, "vector_to_blob", 1, 0, 0, vector_to_blob, 0, 0, 0);
    
    sqlite3_create_function_v2(db, "add_training", 1, 0, (void*) trainingx, add_training, 0, 0, 0);
    sqlite3_create_function_v2(db, "train", 0, 0, (void*) doub, train, 0, 0, 0);

    sqlite3_create_module(db, "templatevtab", &templatevtabModule, 0);
    return 0;
  }
}

#pragma endregion