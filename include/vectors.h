#include <stdint.h>
#include <vector>
#include "sqlite3ext.h"

struct VectorFloat {
  int64_t size;
  float * data;
};
char VECTOR_BLOB_HEADER_BYTE = 'v';
char VECTOR_BLOB_HEADER_TYPE = 1;
const char * VECTOR_FLOAT_POINTER_NAME = "vectorf32v0";


typedef struct vector0_api vector0_api;
struct vector0_api {
  int iVersion;
  std::vector<float>* (*xValueAsVector)(sqlite3_value*value);
  void (*xResultVector)(sqlite3_context*context, std::vector<float>*);
};