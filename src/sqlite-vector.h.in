#ifndef _SQLITE_VECTOR_H
#define _SQLITE_VECTOR_H

#include "sqlite3ext.h"

#ifdef __cplusplus
#include <memory>
#include <stdint.h>
#include <vector>
struct VectorFloat {
    int64_t size;
    float *data;
};

struct vector0_api {

    int iVersion;
    std::unique_ptr<std::vector<float>> (*xValueAsVector)(sqlite3_value *value);
    void (*xResultVector)(sqlite3_context *context, std::vector<float> *);
};

#endif /* end of C++ specific APIs*/

#ifdef __cplusplus
extern "C" {
#endif

int sqlite3_vector_init(sqlite3 *db, char **pzErrMsg,
                        const sqlite3_api_routines *pApi);

#ifdef __cplusplus
} /* end of the 'extern "C"' block */
#endif

#endif /* ifndef _SQLITE_VECTOR_H */
