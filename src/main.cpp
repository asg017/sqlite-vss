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

//int main() {
//  printf("yo\n");
//  return 0;
//}


static void func(sqlite3_context *context, int argc, sqlite3_value **argv) {
faiss::IndexIVFPQ * index = (faiss::IndexIVFPQ *) sqlite3_user_data(context);
  
  int k = 5;
  size_t nq = 1;
  std::vector<float> queries;
  queries.resize(nq * 128);
  std::mt19937 rng;
  std::uniform_real_distribution<> distrib;
  
  for (int j = 0; j < 128; j++) {
    queries[0 * 128 + j] = distrib(rng);
  }
    std::vector<faiss::idx_t> nns(k * nq);
    std::vector<float> dis(k * nq);

    index->search(nq, queries.data(), k, dis.data(), nns.data());
    sqlite3_result_null(context);
}


struct VecX {
  int64_t size;
  double * data;
};
static void faiss_vector_debug(sqlite3_context *context, int argc, sqlite3_value **argv) {
  //printf("sizeof(double)=%d", sizeof(double));
  VecX* v = (VecX*) sqlite3_value_pointer(argv[0], "vecx0");
  //printf("cpp vec: v=%p, v.size=%d , v->data=%p\n", v, v->size, v->data);
  //int N = sizeof(v->data) / sizeof(v->data[0]);

  std::vector<double> vvv(v->data, v->data + v->size);
  /*printf("vvv.size()=%d\n", vvv.size());
  for(double   i : vvv) 
    printf("%f-", i);
  printf("\n");*/


}


void cleanup(void *p){
  //free(p);
}

extern "C" {
  #ifdef _WIN32
  __declspec(dllexport)
  #endif
  int sqlite3_extension_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi);

    // dimension of the vectors to index
    int d = 128;

    // size of the database we plan to index
    size_t nb = 200 * 100;

    // make a set of nt training vectors in the unit cube
    // (could be the database)
    size_t nt = 100 * 100;

    // make the index object and train it
    faiss::IndexFlatL2* coarse_quantizer;//(d);
    coarse_quantizer = new faiss::IndexFlatL2 (d);

    // a reasonable number of centroids to index nb vectors
    int ncentroids = int(4 * sqrt(nb));

    // the coarse quantizer should not be dealloced before the index
    // 4 = nb of bytes per code (d must be a multiple of this)
    // 8 = nb of bits per sub-code (almost always 8)
    faiss::IndexIVFPQ *index;//(&coarse_quantizer, d, ncentroids, 4, 8);
    index = new faiss::IndexIVFPQ(coarse_quantizer, d, ncentroids, 4, 8);

    std::mt19937 rng;

    { // training
        std::vector<float> trainvecs(nt * d);
        std::uniform_real_distribution<> distrib;
        for (size_t i = 0; i < nt * d; i++) {
            trainvecs[i] = distrib(rng);
        }

        //index->verbose = true;
        index->train(nt, trainvecs.data());
    }

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

    
    { // populating the database
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
    }

    {
      int k = 5;
  size_t nq = 1;
  std::vector<float> queries;
  queries.resize(nq * 128);
  std::mt19937 rng;
  std::uniform_real_distribution<> distrib;
  distrib(rng);
  for (int j = 0; j < 128; j++) {
    queries[0 * 128 + j] = distrib(rng);
  }
        std::vector<faiss::idx_t> nns(k * nq);
        std::vector<float> dis(k * nq);

        index->search(nq, queries.data(), k, dis.data(), nns.data());
    }

    
    sqlite3_create_function_v2(db, "a", 0, 0,(void*) index, func, 0, 0, cleanup);
    return sqlite3_create_function_v2(db, "faiss_vector_debug", 1, 0, 0, faiss_vector_debug, 0, 0, 0);
  }
}
