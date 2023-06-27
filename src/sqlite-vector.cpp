
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

#include "vec/functions.h"
#include "vec/fvecsEach_cursor.h"
#include "vec/fvecsEach_vtab.h"

#pragma endregion

#pragma region fvecs vtab

#define FVECS_EACH_DIMENSIONS 0
#define FVECS_EACH_VECTOR 1
#define FVECS_EACH_INPUT 2

static int fvecsEachConnect(sqlite3 *db,
                            void *pAux,
                            int argc,
                            const char *const *argv,
                            sqlite3_vtab **ppVtab,
                            char **pzErr) {

    int rc;

    rc = sqlite3_declare_vtab(db, "create table x(dimensions, vector, input hidden)");

    if (rc == SQLITE_OK) {

        auto pNew = new fvecsEach_vtab();
        if (pNew == nullptr)
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

    pCur->setBlob(sqlite3_malloc(size));
    pCur->iBlobN = size;
    pCur->iRowid = 1;
    memcpy(pCur->getBlob(), blob, size);

    memcpy(&pCur->iCurrentD, pCur->getBlob(), sizeof(int));
    float *vecBegin = (float *)((char *)pCur->getBlob() + sizeof(int));

    // TODO: Shouldn't this multiply by sizeof(float)?
    pCur->pCurrentVector = vec_ptr(new vector<float>(vecBegin, vecBegin + pCur->iCurrentD));

    pCur->p = sizeof(int) + (pCur->iCurrentD * sizeof(float));

    return SQLITE_OK;
}

static int fvecsEachNext(sqlite3_vtab_cursor *cur) {

    auto pCur = static_cast<fvecsEach_cursor *>(cur);

    // TODO: Shouldn't this multiply by sizeof(float)?
    memcpy(&pCur->iCurrentD, ((char *)pCur->getBlob() + pCur->p), sizeof(int));
    float *vecBegin = (float *)(((char *)pCur->getBlob() + pCur->p) + sizeof(int));

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
    /* xUpdate     */ nullptr,
    /* xBegin      */ nullptr,
    /* xSync       */ nullptr,
    /* xCommit     */ nullptr,
    /* xRollback   */ nullptr,
    /* xFindMethod */ nullptr,
    /* xRename     */ nullptr,
    /* xSavepoint  */ nullptr,
    /* xRelease    */ nullptr,
    /* xRollbackTo */ nullptr,
    /* xShadowName */ nullptr
};

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
                                            nullptr,
                                            nullptr,
                                            nullptr);

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
