
#pragma region Includes and typedefs

#include "sqlite-vector.h"
#include "sqlite-vss.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

using namespace std;

typedef unique_ptr<vector<float>> vec_ptr;

// https://github.com/sqlite/sqlite/blob/master/src/json.c#L88-L89
#define JSON_SUBTYPE 74 /* Ascii for "J" */

#include <nlohmann/json.hpp>
using json = nlohmann::json;

char VECTOR_BLOB_HEADER_BYTE = 'v';
char VECTOR_BLOB_HEADER_TYPE = 1;
const char *VECTOR_FLOAT_POINTER_NAME = "vectorf32v0";

#pragma endregion

#pragma region Generic

void delVectorFloat(void *p) {

    auto vx = static_cast<VectorFloat *>(p);
    sqlite3_free(vx->data);
    delete vx;
}

void resultVector(sqlite3_context *context, vector<float> *vecIn) {

    auto vecRes = new VectorFloat();

    vecRes->size = vecIn->size();
    vecRes->data = (float *)sqlite3_malloc(vecIn->size() * sizeof(float));

    memcpy(vecRes->data, vecIn->data(), vecIn->size() * sizeof(float));

    sqlite3_result_pointer(context, vecRes, VECTOR_FLOAT_POINTER_NAME, delVectorFloat);
}

vec_ptr vectorFromBlobValue(sqlite3_value *value, const char **pzErrMsg) {

    int size = sqlite3_value_bytes(value);
    char header;
    char type;

    if (size < (2)) {
        *pzErrMsg = "Vector blob size less than header length";
        return nullptr;
    }

    const void *pBlob = sqlite3_value_blob(value);
    memcpy(&header, ((char *)pBlob + 0), sizeof(char));
    memcpy(&type, ((char *)pBlob + 1), sizeof(char));

    if (header != VECTOR_BLOB_HEADER_BYTE) {
        *pzErrMsg = "Blob not well-formatted vector blob";
        return nullptr;
    }

    if (type != VECTOR_BLOB_HEADER_TYPE) {
        *pzErrMsg = "Blob type not right";
        return nullptr;
    }

    int numElements = (size - 2) / sizeof(float);
    float *vec = (float *)((char *)pBlob + 2);
    return vec_ptr(new vector<float>(vec, vec + numElements));
}

vec_ptr vectorFromRawBlobValue(sqlite3_value *value, const char **pzErrMsg) {

    int size = sqlite3_value_bytes(value);

    // Must be divisible by 4
    if (size % 4) {
        *pzErrMsg = "Invalid raw blob length, blob must be divisible by 4";
        return nullptr;
    }
    const void *pBlob = sqlite3_value_blob(value);

    float *vec = (float *)((char *)pBlob);
    return vec_ptr(new vector<float>(vec, vec + (size / 4)));
}

vec_ptr vectorFromTextValue(sqlite3_value *value) {

    try {

        json json = json::parse(sqlite3_value_text(value));
        vec_ptr pVec(new vector<float>());
        json.get_to(*pVec);
        return pVec;

    } catch (const json::exception &) {
        return nullptr;
    }

    return nullptr;
}

static vec_ptr valueAsVector(sqlite3_value *value) {

    // Option 1: If the value is a "vectorf32v0" pointer, create vector from
    // that
    auto vec = (VectorFloat *)sqlite3_value_pointer(value, VECTOR_FLOAT_POINTER_NAME);

    if (vec != nullptr)
        return vec_ptr(new vector<float>(vec->data, vec->data + vec->size));

    vec_ptr pVec;

    // Option 2: value is a blob in vector format
    if (sqlite3_value_type(value) == SQLITE_BLOB) {

        const char *pzErrMsg = nullptr;

        if ((pVec = vectorFromBlobValue(value, &pzErrMsg)) != nullptr)
            return pVec;

        if ((pVec = vectorFromRawBlobValue(value, &pzErrMsg)) != nullptr)
            return pVec;
    }

    // Option 3: if value is a JSON array coercible to float vector, use that
    if (sqlite3_value_type(value) == SQLITE_TEXT) {

        if ((pVec = vectorFromTextValue(value)) != nullptr)
            return pVec;
        else
            return nullptr;
    }

    // Else, value isn't a vector
    return nullptr;
}

#pragma endregion

#pragma region Meta

static void vector_version(sqlite3_context *context,
                           int argc,
                           sqlite3_value **argv) {

    sqlite3_result_text(context, SQLITE_VSS_VERSION, -1, SQLITE_STATIC);
}

static void vector_debug(sqlite3_context *context,
                         int argc,
                         sqlite3_value **argv) {

    vec_ptr pVec = valueAsVector(argv[0]);

    if (pVec == nullptr) {

        sqlite3_result_error(context, "Value not a vector", -1);
        return;
    }

    sqlite3_str *str = sqlite3_str_new(0);
    sqlite3_str_appendf(str, "size: %lld [", pVec->size());

    for (int i = 0; i < pVec->size(); i++) {

        if (i == 0)
            sqlite3_str_appendf(str, "%f", pVec->at(i));
        else
            sqlite3_str_appendf(str, ", %f", pVec->at(i));
    }

    sqlite3_str_appendchar(str, 1, ']');
    sqlite3_result_text(context, sqlite3_str_finish(str), -1, sqlite3_free);
}

#pragma endregion

#pragma region Vector generation

// TODO should return fvec, ivec, or bvec depending on input. How do bvec,
// though?
static void vector_from(sqlite3_context *context,
                        int argc,
                        sqlite3_value **argv) {

    vector<float> vec;
    vec.reserve(argc);
    for (int i = 0; i < argc; i++) {
        vec.push_back(sqlite3_value_double(argv[i]));
    }

    resultVector(context, &vec);
}

#pragma endregion

#pragma region Vector general

static void vector_value_at(sqlite3_context *context,
                            int argc,
                            sqlite3_value **argv) {

    vec_ptr pVec = valueAsVector(argv[0]);

    if (pVec == nullptr)
        return;

    int pos = sqlite3_value_int(argv[1]);

    try {

        float result = pVec->at(pos);
        sqlite3_result_double(context, result);

    } catch (const out_of_range &oor) {

        char *errmsg = sqlite3_mprintf("%d out of range: %s", pos, oor.what());

        if (errmsg != nullptr) {
            sqlite3_result_error(context, errmsg, -1);
            sqlite3_free(errmsg);
        } else {
            sqlite3_result_error_nomem(context);
        }
    }
}

static void vector_length(sqlite3_context *context,
                          int argc,
                          sqlite3_value **argv) {

    auto pVec = (VectorFloat *)sqlite3_value_pointer(argv[0], VECTOR_FLOAT_POINTER_NAME);
    if (pVec == nullptr)
        return;

    sqlite3_result_int64(context, pVec->size);
}

#pragma endregion

#pragma region Json

static void vector_to_json(sqlite3_context *context,
                           int argc,
                           sqlite3_value **argv) {

    vec_ptr pVec = valueAsVector(argv[0]);
    if (pVec == nullptr)
        return;

    json j = json(*pVec);

    sqlite3_result_text(context, j.dump().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_result_subtype(context, JSON_SUBTYPE);
}

static void vector_from_json(sqlite3_context *context,
                             int argc,
                             sqlite3_value **argv) {

    const char *text = (const char *)sqlite3_value_text(argv[0]);
    vec_ptr pVec = vectorFromTextValue(argv[0]);

    if (pVec == nullptr) {
        sqlite3_result_error(
            context, "input not valid json, or contains non-float data", -1);
    } else {
        resultVector(context, pVec.get());
    }
}

#pragma endregion

#pragma region Blob

/*

|Offset | Size | Description
|-|-|-
|a|a|A
*/
static void vector_to_blob(sqlite3_context *context,
                           int argc,
                           sqlite3_value **argv) {

    vec_ptr pVec = valueAsVector(argv[0]);
    if (pVec == nullptr)
        return;

    int size = pVec->size();
    int memSize = (sizeof(char)) + (sizeof(char)) + (size * 4);
    void *pBlob = sqlite3_malloc(memSize);
    memset(pBlob, 0, memSize);

    memcpy((void *)((char *)pBlob + 0), (void *)&VECTOR_BLOB_HEADER_BYTE, sizeof(char));
    memcpy((void *)((char *)pBlob + 1), (void *)&VECTOR_BLOB_HEADER_TYPE, sizeof(char));
    memcpy((void *)((char *)pBlob + 2), (void *)pVec->data(), size * 4);

    sqlite3_result_blob64(context, pBlob, memSize, sqlite3_free);
}

static void vector_from_blob(sqlite3_context *context,
                             int argc,
                             sqlite3_value **argv) {

    const char *pzErrMsg;

    vec_ptr pVec = vectorFromBlobValue(argv[0], &pzErrMsg);
    if (pVec == nullptr)
        sqlite3_result_error(context, pzErrMsg, -1);
    else
        resultVector(context, pVec.get());
}

static void vector_to_raw(sqlite3_context *context,
                          int argc,
                          sqlite3_value **argv) {

    vec_ptr pVec = valueAsVector(argv[0]);
    if (pVec == nullptr)
        return;

    int size = pVec->size();
    int n = size * sizeof(float);
    void *pBlob = sqlite3_malloc(n);
    memset(pBlob, 0, n);
    memcpy((void *)((char *)pBlob), (void *)pVec->data(), n);
    sqlite3_result_blob64(context, pBlob, n, sqlite3_free);
}

static void vector_from_raw(sqlite3_context *context,
                            int argc,
                            sqlite3_value **argv) {

    const char *pzErrMsg; // TODO: Shouldn't we have like error messages here?

    vec_ptr pVec = vectorFromRawBlobValue(argv[0], &pzErrMsg);
    if (pVec == nullptr)
        sqlite3_result_error(context, pzErrMsg, -1);
    else
        resultVector(context, pVec.get());
}

#pragma endregion

#pragma region fvecs vtab

struct fvecsEach_vtab : public sqlite3_vtab {
  
    fvecsEach_vtab() {

      pModule = nullptr;
      nRef = 0;
      zErrMsg = nullptr;
    }

    ~fvecsEach_vtab() {

        if (zErrMsg != nullptr) {
            sqlite3_free(zErrMsg);
        }
    }
};

struct fvecsEach_cursor : public sqlite3_vtab_cursor {

    fvecsEach_cursor(sqlite3_vtab *pVtab) {
      
      this->pVtab = pVtab;
      iRowid = 0;
      pBlob = nullptr;
      iBlobN = 0;
      p = 0;
      iCurrentD = 0;
    }

    ~fvecsEach_cursor() {
        if (pBlob != nullptr)
            sqlite3_free(pBlob);
    }

    sqlite3_int64 iRowid;

    // Copy of fvecs input blob
    void *pBlob;

    // Total size of pBlob in bytes
    sqlite3_int64 iBlobN;
    sqlite3_int64 p;

    // Current dimensions
    int iCurrentD;

    // Pointer to current vector being read in
    vec_ptr pCurrentVector;
};

static int fvecsEachConnect(sqlite3 *db,
                            void *pAux,
                            int argc,
                            const char *const *argv,
                            sqlite3_vtab **ppVtab,
                            char **pzErr) {

    int rc;

    rc = sqlite3_declare_vtab(db, "create table x(dimensions, vector, input hidden)");

#define FVECS_EACH_DIMENSIONS 0
#define FVECS_EACH_VECTOR 1
#define FVECS_EACH_INPUT 2

    if (rc == SQLITE_OK) {

        auto pNew = new fvecsEach_vtab();
        if (pNew == 0)
            return SQLITE_NOMEM;

        *ppVtab = pNew;
    }
    return rc;
}

static int fvecsEachDisconnect(sqlite3_vtab *pVtab) {

    auto pTable = static_cast<fvecsEach_vtab *>(pVtab);
    delete pTable;
    return SQLITE_OK;
}

static int fvecsEachOpen(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor) {

    auto pCur = new fvecsEach_cursor(p);
    if (pCur == nullptr)
        return SQLITE_NOMEM;

    *ppCursor = pCur;
    return SQLITE_OK;
}

static int fvecsEachClose(sqlite3_vtab_cursor *cur) {

    auto pCur = static_cast<fvecsEach_cursor *>(cur);
    delete pCur;
    return SQLITE_OK;
}

static int fvecsEachBestIndex(sqlite3_vtab *tab, sqlite3_index_info *pIdxInfo) {

    for (int i = 0; i < pIdxInfo->nConstraint; i++) {

        auto pCons = pIdxInfo->aConstraint[i];

        switch (pCons.iColumn) {

            case FVECS_EACH_INPUT:
                if (pCons.op == SQLITE_INDEX_CONSTRAINT_EQ && pCons.usable) {

                    pIdxInfo->aConstraintUsage[i].argvIndex = 1;
                    pIdxInfo->aConstraintUsage[i].omit = 1;
                }
            break;
        }
    }

    pIdxInfo->estimatedCost = (double)10;
    pIdxInfo->estimatedRows = 10;
    return SQLITE_OK;
}

static int fvecsEachFilter(sqlite3_vtab_cursor *pVtabCursor,
                           int idxNum,
                           const char *idxStr,
                           int argc,
                           sqlite3_value **argv) {

    auto pCur = static_cast<fvecsEach_cursor *>(pVtabCursor);

    int size = sqlite3_value_bytes(argv[0]);
    const void *blob = sqlite3_value_blob(argv[0]);

    if (pCur->pBlob)
        sqlite3_free(pCur->pBlob);

    pCur->pBlob = sqlite3_malloc(size);
    pCur->iBlobN = size;
    pCur->iRowid = 1;
    memcpy(pCur->pBlob, blob, size);

    memcpy(&pCur->iCurrentD, pCur->pBlob, sizeof(int));
    float *vecBegin = (float *)((char *)pCur->pBlob + sizeof(int));

    // TODO: Shouldn't this multiply by sizeof(float)?
    pCur->pCurrentVector = vec_ptr(new vector<float>(vecBegin, vecBegin + pCur->iCurrentD));

    pCur->p = sizeof(int) + (pCur->iCurrentD * sizeof(float));

    return SQLITE_OK;
}

static int fvecsEachNext(sqlite3_vtab_cursor *cur) {

    auto pCur = static_cast<fvecsEach_cursor *>(cur);

    // TODO: Shouldn't this multiply by sizeof(float)?
    memcpy(&pCur->iCurrentD, ((char *)pCur->pBlob + pCur->p), sizeof(int));
    float *vecBegin = (float *)(((char *)pCur->pBlob + pCur->p) + sizeof(int));

    pCur->pCurrentVector->clear();
    pCur->pCurrentVector->shrink_to_fit();
    pCur->pCurrentVector->reserve(pCur->iCurrentD);
    pCur->pCurrentVector->insert(pCur->pCurrentVector->begin(),
                                 vecBegin,
                                 vecBegin + pCur->iCurrentD);

    pCur->p += (sizeof(int) + (pCur->iCurrentD * sizeof(float)));
    pCur->iRowid++;
    return SQLITE_OK;
}

static int fvecsEachEof(sqlite3_vtab_cursor *cur) {

    auto pCur = (fvecsEach_cursor *)cur;
    return pCur->p > pCur->iBlobN;
}

static int fvecsEachRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid) {

    fvecsEach_cursor *pCur = (fvecsEach_cursor *)cur;
    *pRowid = pCur->iRowid;
    return SQLITE_OK;
}

static int fvecsEachColumn(sqlite3_vtab_cursor *cur,
                           sqlite3_context *context,
                           int i) {

    auto pCur = static_cast<fvecsEach_cursor *>(cur);

    switch (i) {

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
 * This following structure defines all the methods for the
 * virtual table.
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
    /* xShadowName */ 0};

#pragma endregion

#pragma region Entrypoint

static void vector0(sqlite3_context *context, int argc, sqlite3_value **argv) {

    auto api = (vector0_api *)sqlite3_user_data(context);
    vector0_api **ppApi = (vector0_api **)sqlite3_value_pointer(argv[0], "vector0_api_ptr");
    if (ppApi)
        *ppApi = api;
}

static void delete_api(void * pApi) {

    auto api = static_cast<vector0_api *>(pApi);
    delete api;
}

extern "C" {
#ifdef _WIN32
__declspec(dllexport)
#endif

    int sqlite3_vector_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) {

        int rc = SQLITE_OK;
        SQLITE_EXTENSION_INIT2(pApi);

        auto api = new vector0_api();
        api->iVersion = 0;
        api->xValueAsVector = valueAsVector;
        api->xResultVector = resultVector;

        rc = sqlite3_create_function_v2(db,
                                        "vector0",
                                        1,
                                        SQLITE_UTF8,
                                        api,
                                        vector0,
                                        0,
                                        0,
                                        delete_api);

        if (rc != SQLITE_OK) {

            *pzErrMsg = sqlite3_mprintf("%s: %s", "vector0", sqlite3_errmsg(db));
            return rc;
        }

        static const struct {

            char *zFName;
            int nArg;
            void *pAux;
            void (*xFunc)(sqlite3_context *, int, sqlite3_value **);
            int flags;

        } aFunc[] = {

            { (char *)"vector_version",    0, nullptr, vector_version,   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS },
            { (char *)"vector_debug",      1, nullptr, vector_debug,     SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS },
            { (char *)"vector_length",     1, nullptr, vector_length,    SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS },
            { (char *)"vector_value_at",   2, nullptr, vector_value_at,  SQLITE_UTF8 | SQLITE_INNOCUOUS },
            { (char *)"vector_from_json",  1, nullptr, vector_from_json, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS },
            { (char *)"vector_to_json",    1, nullptr, vector_to_json,   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS },
            { (char *)"vector_from_blob",  1, nullptr, vector_from_blob, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS },
            { (char *)"vector_to_blob",    1, nullptr, vector_to_blob,   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS },
            { (char *)"vector_from_raw",   1, nullptr, vector_from_raw,  SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS },
            { (char *)"vector_to_raw",     1, nullptr, vector_to_raw,    SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS },
        };

        for (int i = 0; i < sizeof(aFunc) / sizeof(aFunc[0]) && rc == SQLITE_OK; i++) {

            rc = sqlite3_create_function_v2(db,
                                            aFunc[i].zFName,
                                            aFunc[i].nArg,
                                            aFunc[i].flags,
                                            aFunc[i].pAux,
                                            aFunc[i].xFunc,
                                            0,
                                            0,
                                            0);

            if (rc != SQLITE_OK) {

                *pzErrMsg = sqlite3_mprintf("%s: %s", aFunc[i].zFName, sqlite3_errmsg(db));
                return rc;
            }
        }

        rc = sqlite3_create_module_v2(db, "vector_fvecs_each", &fvecsEachModule, nullptr, nullptr);
        if (rc != SQLITE_OK) {

            *pzErrMsg = sqlite3_mprintf("%s", sqlite3_errmsg(db));
            return rc;
        }

        return SQLITE_OK;
    }
}

#pragma endregion
