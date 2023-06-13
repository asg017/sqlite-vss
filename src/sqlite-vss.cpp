#include "sqlite-vss.h"
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
#include <faiss/impl/AuxIndexStructures.h>
#include <faiss/impl/IDSelector.h>
#include <faiss/impl/io.h>
#include <faiss/index_factory.h>
#include <faiss/index_io.h>
#include <faiss/utils/distances.h>
#include <faiss/utils/utils.h>

#include "sqlite-vector.h"

using namespace std;

typedef unique_ptr<vector<float>> vec_ptr;

#pragma region meta

static void vss_version(sqlite3_context *context, int argc,
                        sqlite3_value **argv) {

    sqlite3_result_text(context, SQLITE_VSS_VERSION, -1, SQLITE_STATIC);
}

static void vss_debug(sqlite3_context *context, int argc,
                      sqlite3_value **argv) {

    const char *debug = sqlite3_mprintf(
        "version: %s\nfaiss version: %d.%d.%d\nfaiss compile options: %s",
        SQLITE_VSS_VERSION, FAISS_VERSION_MAJOR, FAISS_VERSION_MINOR,
        FAISS_VERSION_PATCH, faiss::get_compile_options().c_str());

    sqlite3_result_text(context, debug, -1, SQLITE_TRANSIENT);
    sqlite3_free((void *)debug);
}

#pragma endregion

#pragma region distances

static void vss_distance_l1(sqlite3_context *context, int argc,
                            sqlite3_value **argv) {

    vector0_api *vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr a = vector_api->xValueAsVector(argv[0]);
    if (a == nullptr) {
        sqlite3_result_error(context, "a is not a vector", -1);
        return;
    }

    vec_ptr b = vector_api->xValueAsVector(argv[1]);
    if (b == nullptr) {
        sqlite3_result_error(context, "b is not a vector", -1);
        return;
    }

    if (a->size() != b->size()) {
        sqlite3_result_error(context, "a and b is not vectors of the same size",
                             -1);
        return;
    }

    int d = a->size();
    sqlite3_result_double(context, faiss::fvec_L1(a->data(), b->data(), d));
}

static void vss_distance_l2(sqlite3_context *context, int argc,
                            sqlite3_value **argv) {

    vector0_api *vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr a = vector_api->xValueAsVector(argv[0]);
    if (a == nullptr) {
        sqlite3_result_error(context, "a is not a vector", -1);
        return;
    }

    vec_ptr b = vector_api->xValueAsVector(argv[1]);
    if (b == nullptr) {
        sqlite3_result_error(context, "b is not a vector", -1);
        return;
    }

    if (a->size() != b->size()) {
        sqlite3_result_error(context, "a and b is not vectors of the same size",
                             -1);
        return;
    }

    int d = a->size();
    sqlite3_result_double(context, faiss::fvec_L2sqr(a->data(), b->data(), d));
}

static void vss_distance_linf(sqlite3_context *context, int argc,
                              sqlite3_value **argv) {

    vector0_api *vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr a = vector_api->xValueAsVector(argv[0]);
    if (a == nullptr) {
        sqlite3_result_error(context, "a is not a vector", -1);
        return;
    }

    vec_ptr b = vector_api->xValueAsVector(argv[1]);
    if (b == nullptr) {
        sqlite3_result_error(context, "b is not a vector", -1);
        return;
    }

    if (a->size() != b->size()) {
        sqlite3_result_error(context, "a and b is not vectors of the same size",
                             -1);
        return;
    }

    int d = a->size();
    sqlite3_result_double(context, faiss::fvec_Linf(a->data(), b->data(), d));
}

static void vss_inner_product(sqlite3_context *context, int argc,
                              sqlite3_value **argv) {

    vector0_api *vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr a = vector_api->xValueAsVector(argv[0]);
    if (a == nullptr) {
        sqlite3_result_error(context, "a is not a vector", -1);
        return;
    }

    vec_ptr b = vector_api->xValueAsVector(argv[1]);
    if (b == nullptr) {
        sqlite3_result_error(context, "b is not a vector", -1);
        return;
    }

    if (a->size() != b->size()) {
        sqlite3_result_error(context, "a and b is not vectors of the same size",
                             -1);
        return;
    }

    int d = a->size();
    sqlite3_result_double(context,
                          faiss::fvec_inner_product(a->data(), b->data(), d));
}

static void vss_fvec_add(sqlite3_context *context, int argc,
                         sqlite3_value **argv) {

    vector0_api *vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr a = vector_api->xValueAsVector(argv[0]);
    if (a == nullptr) {
        sqlite3_result_error(context, "a is not a vector", -1);
        return;
    }

    vec_ptr b = vector_api->xValueAsVector(argv[1]);
    if (b == nullptr) {
        sqlite3_result_error(context, "b is not a vector", -1);
        return;
    }

    if (a->size() != b->size()) {
        sqlite3_result_error(context, "a and b is not vectors of the same size",
                             -1);
        return;
    }

    int d = a->size();
    vec_ptr c(new vector<float>(d));
    faiss::fvec_add(d, a->data(), b->data(), c->data());

    vector_api->xResultVector(context, c.get());
}

static void vss_fvec_sub(sqlite3_context *context, int argc,
                         sqlite3_value **argv) {

    vector0_api *vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr a = vector_api->xValueAsVector(argv[0]);
    if (a == nullptr) {
        sqlite3_result_error(context, "a is not a vector", -1);
        return;
    }

    vec_ptr b = vector_api->xValueAsVector(argv[1]);
    if (b == nullptr) {
        sqlite3_result_error(context, "b is not a vector", -1);
        return;
    }

    if (a->size() != b->size()) {
        sqlite3_result_error(context, "a and b is not vectors of the same size",
                             -1);
        return;
    }

    int d = a->size();
    vec_ptr c = vec_ptr(new vector<float>(d));
    faiss::fvec_sub(d, a->data(), b->data(), c->data());
    vector_api->xResultVector(context, c.get());
}

#pragma endregion

#pragma region structs and cleanup functions

struct VssSearchParams {

    vec_ptr vector;
    sqlite3_int64 k;
};

void delVssSearchParams(void *p) {

    VssSearchParams *vs = (VssSearchParams *)p;
    delete vs;
}

struct VssRangeSearchParams {

    vec_ptr vector;
    float distance;
};

void delVssRangeSearchParams(void *p) {

    VssRangeSearchParams *vs = (VssRangeSearchParams *)p;
    delete vs;
}

#pragma endregion

#pragma region vtab

static void VssSearchParamsFunc(sqlite3_context *context, int argc,
                                sqlite3_value **argv) {

    vector0_api *vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr vector = vector_api->xValueAsVector(argv[0]);
    if (vector == nullptr) {
        sqlite3_result_error(context, "1st argument is not a vector", -1);
        return;
    }

    sqlite3_int64 k = sqlite3_value_int64(argv[1]);
    VssSearchParams *params = new VssSearchParams();
    params->vector = vec_ptr(vector.release());
    params->k = k;
    sqlite3_result_pointer(context, params, "vss0_searchparams",
                           delVssSearchParams);
}

static void VssRangeSearchParamsFunc(sqlite3_context *context, int argc,
                                     sqlite3_value **argv) {

    vector0_api *vector_api = (vector0_api *)sqlite3_user_data(context);
    vec_ptr vector = vector_api->xValueAsVector(argv[0]);
    if (vector == nullptr) {
        sqlite3_result_error(context, "1st argument is not a vector", -1);
        return;
    }
    float distance = sqlite3_value_double(argv[1]);
    VssRangeSearchParams *params = new VssRangeSearchParams();
    params->vector = vec_ptr(vector.release());
    params->distance = distance;
    sqlite3_result_pointer(context, params, "vss0_rangesearchparams",
                           delVssRangeSearchParams);
}

static int write_index_insert(faiss::Index *index, sqlite3 *db, char *schema,
                              char *name, int i) {

    faiss::VectorIOWriter writer;
    faiss::write_index(index, &writer);
    sqlite3_int64 nIn = writer.data.size();

    // First try to insert into xyz_index. If that fails with a rowid constraint
    // error, that means the index is already on disk, we just have to UPDATE
    // instead.

    sqlite3_stmt *stmt;
    char *sql = sqlite3_mprintf(
        "insert into \"%w\".\"%w_index\"(rowid, idx) values (?, ?)", schema,
        name);

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK || stmt == nullptr) {
        sqlite3_free(sql);
        return SQLITE_ERROR;
    }

    rc = sqlite3_bind_int64(stmt, 1, i);
    rc =
        sqlite3_bind_blob64(stmt, 2, writer.data.data(), nIn, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        sqlite3_free(sql);
        sqlite3_finalize(stmt);
        return SQLITE_ERROR;
    }

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_free(sql);

    // INSERT was success, index wasn't written yet, all good to exit
    if (result == SQLITE_DONE) {
        return SQLITE_OK;
    }
    // INSERT failed for another unknown reason, bad, return error
    else if (sqlite3_extended_errcode(db) != SQLITE_CONSTRAINT_ROWID) {
        return SQLITE_ERROR;
    }

    // INSERT failed because index already is on disk, so do UPDATE instead

    sql = sqlite3_mprintf(
        "UPDATE \"%w\".\"%w_index\" SET idx = ? WHERE rowid = ?", schema, name);
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK || stmt == nullptr) {
        sqlite3_free(sql);
        return SQLITE_ERROR;
    }

    rc = sqlite3_bind_blob64(stmt, 1, writer.data.data(), nIn, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        sqlite3_free(sql);
        return SQLITE_ERROR;
    }

    rc = sqlite3_bind_int64(stmt, 2, i);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        sqlite3_free(sql);
        return SQLITE_ERROR;
    }

    result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_free(sql);

    if (result == SQLITE_DONE) {
        return SQLITE_OK;
    }
    return result;
}

static int shadow_data_insert(sqlite3 *db, char *schema, char *name,
                              sqlite3_int64 *rowid, sqlite3_int64 *retRowid) {

    sqlite3_stmt *stmt;

    if (rowid == nullptr) {

        char *sql = sqlite3_mprintf(
            "insert into \"%w\".\"%w_data\"(x) values (?)", schema, name);

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        sqlite3_free(sql);

        if (rc != SQLITE_OK || stmt == nullptr) {
            return SQLITE_ERROR;
        }

        sqlite3_bind_null(stmt, 1);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            return SQLITE_ERROR;
        }
    } else {

        char *sql = sqlite3_mprintf(
            "insert into \"%w\".\"%w_data\"(rowid, x) values (?, ?);", schema,
            name);

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        sqlite3_free(sql);

        if (rc != SQLITE_OK || stmt == 0) {
            return SQLITE_ERROR;
        }

        sqlite3_bind_int64(stmt, 1, *rowid);
        sqlite3_bind_null(stmt, 2);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            return SQLITE_ERROR;
        }
        if (retRowid != nullptr)
            *retRowid = sqlite3_last_insert_rowid(db);
    }
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

static int shadow_data_delete(sqlite3 *db, char *schema, char *name,
                              sqlite3_int64 rowid) {
    sqlite3_stmt *stmt;

    // TODO: We should strive to use only one concept and idea while creating
    // SQL statements.
    sqlite3_str *query = sqlite3_str_new(0);

    sqlite3_str_appendf(query, "delete from \"%w\".\"%w_data\" where rowid = ?",
                        schema, name);

    char *sql = sqlite3_str_finish(query);
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK || stmt == nullptr) {
        return SQLITE_ERROR;
    }

    sqlite3_bind_int64(stmt, 1, rowid);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return SQLITE_ERROR;
    }

    sqlite3_free(sql);
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

static faiss::Index *read_index_select(sqlite3 *db, const char *name, int i) {

    sqlite3_stmt *stmt;
    char *sql =
        sqlite3_mprintf("select idx from \"%w_index\" where rowid = ?", name);

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK || stmt == nullptr) {
        sqlite3_finalize(stmt);
        sqlite3_free(sql);
        return nullptr;
    }

    sqlite3_bind_int64(stmt, 1, i);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        sqlite3_free(sql);
        return nullptr;
    }

    const void *index_data = sqlite3_column_blob(stmt, 0);
    int64_t n = sqlite3_column_bytes(stmt, 0);

    faiss::VectorIOReader reader;
    copy((const uint8_t *)index_data, ((const uint8_t *)index_data) + n,
              back_inserter(reader.data));

    sqlite3_free(sql);
    sqlite3_finalize(stmt);

    return faiss::read_index(&reader);
}

static int create_shadow_tables(sqlite3 *db, const char *schema,
                                const char *name, int n) {

    char *sql = sqlite3_mprintf("CREATE TABLE \"%w\".\"%w_index\"(idx)", schema,
                                name); // sqlite3_str_finish(create_index_str);
    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    sqlite3_free(sql);
    if (rc != SQLITE_OK)
        return rc;

    sql = sqlite3_mprintf("CREATE TABLE \"%w\".\"%w_data\"(x);", schema, name);
    rc = sqlite3_exec(db, sql, 0, 0, 0);
    sqlite3_free(sql);
    return rc;
}

static int drop_shadow_tables(sqlite3 *db, char *name) {

    const char *drops[2] = {"drop table \"%w_index\";",
                            "drop table \"%w_data\";"};

    for (int i = 0; i < 2; i++) {

        const char *s = drops[i];

        sqlite3_stmt *stmt;

        // TODO: Use of one construct to create SQL statements.
        sqlite3_str *query = sqlite3_str_new(0);
        sqlite3_str_appendf(query, s, name);
        char *sql = sqlite3_str_finish(query);

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        if (rc != SQLITE_OK || stmt == nullptr) {
            sqlite3_free(sql);
            return SQLITE_ERROR;
        }

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_free(sql);
            sqlite3_finalize(stmt);
            return SQLITE_ERROR;
        }

        sqlite3_free(sql);
        sqlite3_finalize(stmt);
    }
    return SQLITE_OK;
}

#define VSS_SEARCH_FUNCTION SQLITE_INDEX_CONSTRAINT_FUNCTION
#define VSS_RANGE_SEARCH_FUNCTION SQLITE_INDEX_CONSTRAINT_FUNCTION + 1

// Wrapper around a single faiss index, with training data, insert records, and
// delete records.
struct vss_index {

    vss_index(faiss::Index *index) : index(index) {}

    ~vss_index() {
        if (index != nullptr)
            delete index;
    }

    faiss::Index *index;
    vector<float> trainings;
    vector<float> insert_to_add_data;
    vector<faiss::idx_t> insert_to_add_ids;
    vector<faiss::idx_t> delete_to_delete_ids;
};

struct vss_index_vtab {

    sqlite3_vtab base; // Base class - must be first
    sqlite3 *db;
    vector0_api *vector_api;

    // Name of the virtual table. Must be freed during disconnect
    char *name;

    // Name of the schema the virtual table exists in. Must be freed during
    // disconnect
    char *schema;

    // Vector holding all the  faiss Indices the vtab uses, and their state,
    // implying which items are to be deleted and inserted.
    vector<vss_index> indexes;

    // Whether the current transaction is inserting training data for at least 1
    // column
    bool isTraining;

    // Whether the current transaction is inserting data for at least 1 column
    bool isInsertData;
};

enum QueryType { search, range_search, fullscan };

struct vss_index_cursor {

    sqlite3_vtab_cursor base; // Base class - must be first
    vss_index_vtab *table;

    sqlite3_int64 iCurrent;
    sqlite3_int64 iRowid;

    QueryType query_type;

    // For query_type == QueryType::search
    sqlite3_int64 search_k;
    vector<faiss::idx_t> search_ids;
    vector<float> search_distances;

    // For query_type == QueryType::range_search
    faiss::RangeSearchResult *range_search_result;

    // For query_type == QueryType::fullscan
    sqlite3_stmt *stmt;
    int step_result;
};

struct VssIndexColumn {

    string name;
    sqlite3_int64 dimensions;
    string factory;
};

vector<VssIndexColumn> *parse_constructor(int argc,
                                               const char *const *argv) {

    vector<VssIndexColumn> *columns = new vector<VssIndexColumn>();

    for (int i = 3; i < argc; i++) {

        string arg = string(argv[i]);

        size_t lparen = arg.find("(");
        size_t rparen = arg.find(")");

        if (lparen == string::npos || rparen == string::npos ||
            lparen >= rparen) {
            return nullptr;
        }

        string name = arg.substr(0, lparen);
        string sDimensions = arg.substr(lparen + 1, rparen - lparen - 1);

        sqlite3_int64 dimensions = atoi(sDimensions.c_str());

        size_t factoryStart, factoryStringStartFrom;
        string factory;

        if ((factoryStart = arg.find("factory", rparen)) != string::npos &&
            (factoryStringStartFrom = arg.find("=", factoryStart)) !=
                string::npos) {

            size_t lquote = arg.find("\"", factoryStringStartFrom);
            size_t rquote = arg.find_last_of("\"");

            if (lquote == string::npos || rquote == string::npos ||
                lquote >= rquote) {
                delete columns;
                return nullptr;
            }
            factory = arg.substr(lquote + 1, rquote - lquote - 1);

        } else {
            factory = string("Flat,IDMap2");
        }
        columns->push_back(VssIndexColumn{name, dimensions, factory});
    }
    // TODO
    return columns;
}

static int init(sqlite3 *db, void *pAux, int argc, const char *const *argv,
                sqlite3_vtab **ppVtab, char **pzErr, bool isCreate) {

    sqlite3_vtab_config(db, SQLITE_VTAB_CONSTRAINT_SUPPORT, 1);
    int rc;

    sqlite3_str *str = sqlite3_str_new(nullptr);
    sqlite3_str_appendall(str,
                          "CREATE TABLE x(distance hidden, operation hidden");

    vector<VssIndexColumn> *columns = parse_constructor(argc, argv);
    if (columns == nullptr) {
        *pzErr = sqlite3_mprintf("Error parsing constructor");
        return rc;
    }

    for (auto column = columns->begin(); column != columns->end(); ++column) {
        sqlite3_str_appendf(str, ", \"%w\"", column->name.c_str());
    }

    sqlite3_str_appendall(str, ")");
    const char *query = sqlite3_str_finish(str);
    rc = sqlite3_declare_vtab(db, query);
    sqlite3_free((void *)query);

#define VSS_INDEX_COLUMN_DISTANCE 0
#define VSS_INDEX_COLUMN_OPERATION 1
#define VSS_INDEX_COLUMN_VECTORS 2

    if (rc != SQLITE_OK)
        return rc;

    vss_index_vtab *pNew;
    pNew = (vss_index_vtab *)sqlite3_malloc(sizeof(*pNew));
    *ppVtab = (sqlite3_vtab *)pNew;
    memset(pNew, 0, sizeof(*pNew));

    pNew->vector_api = (vector0_api *)pAux;

    pNew->schema = sqlite3_mprintf("%s", argv[1]);
    pNew->name = sqlite3_mprintf("%s", argv[2]);
    pNew->db = db;

    if (isCreate) {

        for (auto column = columns->begin(); column != columns->end();
             ++column) {

            try {

                faiss::Index *index = faiss::index_factory(
                    column->dimensions, column->factory.c_str());
                pNew->indexes.push_back(index);

            } catch (faiss::FaissException &e) {

                *pzErr =
                    sqlite3_mprintf("Error building index factory for %s: %s",
                                    column->name.c_str(), e.msg.c_str());

                return SQLITE_ERROR;
            }
        }
        create_shadow_tables(db, argv[1], argv[2], columns->size());

        // after shadow tables are created, write the initial index state to
        // shadow _index
        for (int i = 0; i < pNew->indexes.size(); i++) {

            auto index = pNew->indexes.at(i).index;
            try {

                int rc = write_index_insert(index, pNew->db, pNew->schema,
                                            pNew->name, i);
                if (rc != SQLITE_OK)
                    return rc;

            } catch (faiss::FaissException &e) {
                return SQLITE_ERROR;
            }
        }

    } else {

        for (int i = 0; i < columns->size(); i++) {

            auto index = read_index_select(db, argv[2], i);

            // index in shadow table should always be available, integrity check
            // to avoid null pointer
            if (index == nullptr) {
                *pzErr =
                    sqlite3_mprintf("Could not read index at position %d", i);
                return SQLITE_ERROR;
            }
            pNew->indexes.push_back(vss_index(index));
        }
    }

    pNew->isTraining = false;
    pNew->isInsertData = false;

    return SQLITE_OK;
}

static int vssIndexCreate(sqlite3 *db, void *pAux, int argc,
                          const char *const *argv, sqlite3_vtab **ppVtab,
                          char **pzErr) {

    return init(db, pAux, argc, argv, ppVtab, pzErr, true);
}

static int vssIndexConnect(sqlite3 *db, void *pAux, int argc,
                           const char *const *argv, sqlite3_vtab **ppVtab,
                           char **pzErr) {

    return init(db, pAux, argc, argv, ppVtab, pzErr, false);
}

static int vssIndexDisconnect(sqlite3_vtab *pVtab) {

    vss_index_vtab *p = (vss_index_vtab *)pVtab;

    sqlite3_free(p->name);
    sqlite3_free(p->schema);

    sqlite3_free(p);
    return SQLITE_OK;
}

static int vssIndexDestroy(sqlite3_vtab *pVtab) {

    vss_index_vtab *p = (vss_index_vtab *)pVtab;

    drop_shadow_tables(p->db, p->name);
    vssIndexDisconnect(pVtab);

    return SQLITE_OK;
}

static int vssIndexOpen(sqlite3_vtab *pVtab, sqlite3_vtab_cursor **ppCursor) {

    vss_index_vtab *p = (vss_index_vtab *)pVtab;
    vss_index_cursor * pCur = (vss_index_cursor *)sqlite3_malloc(sizeof(*pCur));
    if (pCur == 0)
        return SQLITE_NOMEM;
    memset(pCur, 0, sizeof(*pCur));

    *ppCursor = &pCur->base;
    pCur->table = p;
    // pCur->nns = new vector<faiss::idx_t>(5);
    // pCur->dis = new vector<float>(5);

    return SQLITE_OK;
}

static int vssIndexClose(sqlite3_vtab_cursor *cur) {

    vss_index_cursor *pCur = (vss_index_cursor *)cur;

    if (pCur->range_search_result) {
        delete pCur->range_search_result;
        pCur->range_search_result = nullptr;
    }

    if (pCur->stmt)
        sqlite3_finalize(pCur->stmt);

    sqlite3_free(pCur);
    return SQLITE_OK;
}

static int vssIndexBestIndex(sqlite3_vtab *tab, sqlite3_index_info *pIdxInfo) {
    int iSearchTerm = -1;
    int iRangeSearchTerm = -1;
    int iXSearchColumn = -1;
    int iLimit = -1;
    // printf("best index, %d\n", pIdxInfo->nConstraint);

    for (int i = 0; i < pIdxInfo->nConstraint; i++) {
        auto constraint = pIdxInfo->aConstraint[i];
        // printf("\t[%d] col=%d, op=%d \n", i,
        // pIdxInfo->aConstraint[i].iColumn, pIdxInfo->aConstraint[i].op);

        if (!constraint.usable)
            continue;
        if (constraint.op == VSS_SEARCH_FUNCTION) {
            iSearchTerm = i;
            iXSearchColumn = constraint.iColumn;
        } else if (constraint.op == VSS_RANGE_SEARCH_FUNCTION) {
            iRangeSearchTerm = i;
            iXSearchColumn = constraint.iColumn;
        } else if (constraint.op == SQLITE_INDEX_CONSTRAINT_LIMIT) {
            iLimit = i;
        }
    }
    if (iSearchTerm >= 0) {
        pIdxInfo->idxNum = iXSearchColumn - VSS_INDEX_COLUMN_VECTORS;
        pIdxInfo->idxStr = (char *)"search";
        pIdxInfo->aConstraintUsage[iSearchTerm].argvIndex = 1;
        pIdxInfo->aConstraintUsage[iSearchTerm].omit = 1;
        if (iLimit >= 0) {
            pIdxInfo->aConstraintUsage[iLimit].argvIndex = 2;
            pIdxInfo->aConstraintUsage[iLimit].omit = 1;
        }
        pIdxInfo->estimatedCost = 300.0;
        pIdxInfo->estimatedRows = 10;
        return SQLITE_OK;
    }
    if (iRangeSearchTerm >= 0) {
        pIdxInfo->idxNum = iXSearchColumn - VSS_INDEX_COLUMN_VECTORS;
        pIdxInfo->idxStr = (char *)"range_search";
        pIdxInfo->aConstraintUsage[iRangeSearchTerm].argvIndex = 1;
        pIdxInfo->aConstraintUsage[iRangeSearchTerm].omit = 1;
        pIdxInfo->estimatedCost = 300.0;
        pIdxInfo->estimatedRows = 10;
        return SQLITE_OK;
    }
    pIdxInfo->idxNum = -1;
    pIdxInfo->idxStr = (char *)"fullscan";
    pIdxInfo->estimatedCost = 3000000.0;
    pIdxInfo->estimatedRows = 100000;
    return SQLITE_OK;
}

static int vssIndexFilter(sqlite3_vtab_cursor *pVtabCursor, int idxNum,
                          const char *idxStr, int argc, sqlite3_value **argv) {

    vss_index_cursor *pCur = (vss_index_cursor *)pVtabCursor;
    if (strcmp(idxStr, "search") == 0) {

        pCur->query_type = QueryType::search;
        vec_ptr query_vector;

        VssSearchParams *params;
        if ((params = (VssSearchParams *)sqlite3_value_pointer(
                 argv[0], "vss0_searchparams")) != nullptr) {

            pCur->search_k = params->k;
            query_vector = vec_ptr(new vector<float>(*params->vector));

        } else if (sqlite3_libversion_number() < 3041000) {

            // https://sqlite.org/forum/info/6b32f818ba1d97ef
            sqlite3_free(pVtabCursor->pVtab->zErrMsg);
            pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf(
                "vss_search() only support vss_search_params() as a "
                "2nd parameter for SQLite versions below 3.41.0");
            return SQLITE_ERROR;

        } else if ((query_vector = pCur->table->vector_api->xValueAsVector(
                        argv[0])) != nullptr) {

            if (argc > 1) {
                pCur->search_k = sqlite3_value_int(argv[1]);
            } else {
                sqlite3_free(pVtabCursor->pVtab->zErrMsg);
                pVtabCursor->pVtab->zErrMsg =
                    sqlite3_mprintf("LIMIT required on vss_search() queries");
                return SQLITE_ERROR;
            }

        } else {

            sqlite3_free(pVtabCursor->pVtab->zErrMsg);
            pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf(
                "2nd argument to vss_search() must be a vector");
            return SQLITE_ERROR;

        }

        int nq = 1;
        faiss::Index *index = pCur->table->indexes.at(idxNum).index;

        if (query_vector->size() != index->d) {

            sqlite3_free(pVtabCursor->pVtab->zErrMsg);
            pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf(
                "input query size doesn't match index dimensions: %ld != %ld",
                query_vector->size(), index->d);
            return SQLITE_ERROR;
        }

        if (pCur->search_k <= 0) {

            sqlite3_free(pVtabCursor->pVtab->zErrMsg);
            pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf(
                "k must be greater than 0, got %ld", pCur->search_k);
            return SQLITE_ERROR;
        }

        pCur->search_distances.reserve(pCur->search_k * nq);

        index->search(nq, query_vector->data(), pCur->search_k,
                      pCur->search_distances.data(), pCur->search_ids.data());

    } else if (strcmp(idxStr, "range_search") == 0) {

        pCur->query_type = QueryType::range_search;

        VssRangeSearchParams *params =
            (VssRangeSearchParams *)sqlite3_value_pointer(
                argv[0], "vss0_rangesearchparams");

        int nq = 1;

        vector<faiss::idx_t> nns(params->distance * nq);
        faiss::RangeSearchResult *result =
            new faiss::RangeSearchResult(nq, true);

        faiss::Index *index = pCur->table->indexes.at(idxNum).index;

        index->range_search(nq, params->vector->data(), params->distance,
                            result);

        pCur->range_search_result = result;

    } else if (strcmp(idxStr, "fullscan") == 0) {

        pCur->query_type = QueryType::fullscan;
        sqlite3_stmt *stmt;

        int res = sqlite3_prepare_v2(
            pCur->table->db,
            sqlite3_mprintf("select rowid from \"%w_data\"", pCur->table->name),
            -1, &pCur->stmt, nullptr);

        if (res != SQLITE_OK)
            return res;
        pCur->step_result = sqlite3_step(pCur->stmt);

    } else {

        if (pVtabCursor->pVtab->zErrMsg != 0)
            sqlite3_free(pVtabCursor->pVtab->zErrMsg);

        pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf(
            "%s %s", "vssIndexFilter error: unhandled idxStr", idxStr);
        return SQLITE_ERROR;
    }

    pCur->iCurrent = 0;
    return SQLITE_OK;
}

static int vssIndexNext(sqlite3_vtab_cursor *cur) {
    vss_index_cursor *pCur = (vss_index_cursor *)cur;
    switch (pCur->query_type) {
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
static int vssIndexRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid) {
    vss_index_cursor *pCur = (vss_index_cursor *)cur;

    switch (pCur->query_type) {

        case QueryType::search:
            *pRowid = pCur->search_ids.at(pCur->iCurrent);
            break;

        case QueryType::range_search:
            *pRowid = pCur->range_search_result->labels[pCur->iCurrent];
            break;

        case QueryType::fullscan:
            *pRowid = sqlite3_column_int64(pCur->stmt, 0);
            break;
    }
    return SQLITE_OK;
}

static int vssIndexEof(sqlite3_vtab_cursor *cur) {
    vss_index_cursor *pCur = (vss_index_cursor *)cur;
    switch (pCur->query_type) {
    case QueryType::search: {
        return pCur->iCurrent >= pCur->search_k ||
               (pCur->search_ids.at(pCur->iCurrent) == -1);
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

static int vssIndexColumn(sqlite3_vtab_cursor *cur, sqlite3_context *ctx,
                          int i) {

    vss_index_cursor *pCur = (vss_index_cursor *)cur;

    if (i == VSS_INDEX_COLUMN_DISTANCE) {

        switch (pCur->query_type) {

          case QueryType::search:
              sqlite3_result_double(ctx,
                                    pCur->search_distances.at(pCur->iCurrent));
              break;

          case QueryType::range_search:
              sqlite3_result_double(
                  ctx, pCur->range_search_result->distances[pCur->iCurrent]);
              break;

          case QueryType::fullscan:
              break;
        }

    } else if (i >= VSS_INDEX_COLUMN_VECTORS) {

        faiss::Index *index =
            pCur->table->indexes.at(i - VSS_INDEX_COLUMN_VECTORS).index;

        vector<float> v(index->d);
        sqlite3_int64 rowid;
        vssIndexRowid(cur, &rowid);

        try {
            index->reconstruct(rowid, v.data());

        } catch (faiss::FaissException &e) {

            char *errmsg =
                (char *)sqlite3_mprintf("Error reconstructing vector - Does "
                                        "the column factory string end "
                                        "with IDMap2? Full error: %s",
                                        e.msg.c_str());

            sqlite3_result_error(ctx, errmsg, -1);
            sqlite3_free(errmsg);
            return SQLITE_ERROR;
        }
        pCur->table->vector_api->xResultVector(ctx, &v);
    }
    return SQLITE_OK;
}

static int vssIndexBegin(sqlite3_vtab *tab) {

    return SQLITE_OK;
}

static int vssIndexSync(sqlite3_vtab *pVTab) {

    vss_index_vtab *p = (vss_index_vtab *)pVTab;

    bool needsWriting = false;
    if (p->isTraining) {

        for (size_t i = 0; i != p->indexes.size(); ++i) {
            auto &training = p->indexes.at(i).trainings;
            if (!training.empty()) {
                faiss::Index *index = p->indexes.at(i).index;
                index->train(training.size() / index->d, training.data());
                training.clear();
            }
        }
        p->isTraining = false;
    }

    for (auto iter = p->indexes.begin(); iter != p->indexes.end(); ++iter) {

        if (!iter->delete_to_delete_ids.empty()) {

            faiss::IDSelectorBatch selector(iter->delete_to_delete_ids.size(),
                                            iter->delete_to_delete_ids.data());
            size_t numRemoved = iter->index->remove_ids(selector);
            needsWriting = true;
            iter->delete_to_delete_ids.clear();
        }
    }

    if (p->isInsertData) {

        p->isInsertData = false;

        int i = 0;
        for (auto iter = p->indexes.begin(); iter != p->indexes.end(); ++iter, i++) {

            auto &insert_data = iter->insert_to_add_data;
            auto &insert_ids = iter->insert_to_add_ids;

            if (!insert_data.empty()) {

                try {

                    iter->index->add_with_ids(
                        insert_ids.size(), insert_data.data(),
                        (faiss::idx_t *)insert_ids.data());

                } catch (faiss::FaissException &e) {

                    sqlite3_free(pVTab->zErrMsg);
                    pVTab->zErrMsg =
                        sqlite3_mprintf("Error adding vector to index at "
                                        "column index %d. Full error: %s",
                                        i, e.msg.c_str());

                    insert_ids.clear();
                    insert_data.clear();

                    return SQLITE_ERROR;
                }

                insert_ids.clear();
                insert_data.clear();
                insert_ids.shrink_to_fit();
                insert_data.shrink_to_fit();
                needsWriting = true;
            }
        }
    }

    if (needsWriting) {

        int i = 0;
        for (auto iter = p->indexes.begin(); iter != p->indexes.end(); ++iter, i++) {

            int rc = write_index_insert(iter->index, p->db,
                                        p->schema, p->name, i);

            if (rc != SQLITE_OK) {
                sqlite3_free(pVTab->zErrMsg);
                pVTab->zErrMsg = sqlite3_mprintf("Error saving index (%d): %s",
                                                 rc, sqlite3_errmsg(p->db));
                return rc;
            }
        }
    }
    return SQLITE_OK;
}

static int vssIndexCommit(sqlite3_vtab *pVTab) { return SQLITE_OK; }

static int vssIndexRollback(sqlite3_vtab *pVTab) {

    vss_index_vtab *p = (vss_index_vtab *)pVTab;

    for (size_t i = 0; i != p->indexes.size(); ++i) {
        p->indexes.at(i).trainings.clear();
    }

    //for (size_t i = 0; i < p->indexCount; ++i) {
    for (auto iter = p->indexes.begin(); iter != p->indexes.end(); ++iter) {
        iter->insert_to_add_data.clear();
        iter->insert_to_add_ids.clear();
        iter->delete_to_delete_ids.clear();
    }
    return SQLITE_OK;
}

static int vssIndexUpdate(sqlite3_vtab *pVTab, int argc, sqlite3_value **argv,
                          sqlite_int64 *pRowid) {

    vss_index_vtab *p = (vss_index_vtab *)pVTab;

    if (argc == 1 && sqlite3_value_type(argv[0]) != SQLITE_NULL) {

        // DELETE operation
        sqlite3_int64 rowid_to_delete = sqlite3_value_int64(argv[0]);

        int rc;
        if ((rc = shadow_data_delete(p->db, p->schema, p->name,
                                     rowid_to_delete)) != SQLITE_OK) {
            return rc;
        }

        for (auto iter = p->indexes.begin(); iter != p->indexes.end(); ++iter) {
            iter->delete_to_delete_ids.push_back(rowid_to_delete);
        }

    } else if (argc > 1 && sqlite3_value_type(argv[0]) == SQLITE_NULL) {

        // INSERT operation
        // if no operation, we add it to the index
        bool noOperation =
            sqlite3_value_type(argv[2 + VSS_INDEX_COLUMN_OPERATION]) ==
            SQLITE_NULL;

        if (noOperation) {

            vec_ptr vec;
            sqlite3_int64 rowid = sqlite3_value_int64(argv[1]);
            bool inserted_rowid = false;

            auto i = 0;
            for (auto iter = p->indexes.begin(); iter != p->indexes.end(); ++iter, i++) {

                if ((vec = p->vector_api->xValueAsVector(
                         argv[2 + VSS_INDEX_COLUMN_VECTORS + i])) != nullptr) {

                    // make sure the index is already trained, if it's needed
                    if (!iter->index->is_trained) {

                        sqlite3_free(pVTab->zErrMsg);
                        pVTab->zErrMsg =
                            sqlite3_mprintf("Index at i=%d requires training "
                                            "before inserting data.",
                                            i);

                        return SQLITE_ERROR;
                    }

                    if (!inserted_rowid) {

                        sqlite_int64 retrowid;
                        int rc = shadow_data_insert(p->db, p->schema, p->name,
                                                    &rowid, &retrowid);
                        if (rc != SQLITE_OK)
                            return rc;

                        inserted_rowid = true;
                    }

                    iter->insert_to_add_data.reserve(
                        vec->size() + distance(vec->begin(), vec->end())); // TODO: Doesn't this grow by twice as much as needed?

                    iter->insert_to_add_data.insert(
                        iter->insert_to_add_data.end(), vec->begin(),
                        vec->end());

                    iter->insert_to_add_ids.push_back(rowid);

                    p->isInsertData = true;
                    *pRowid = rowid;
                }
            }

        } else {

            // TODO: Won't this this leak the char *?
            string operation((char *)sqlite3_value_text(
                argv[2 + VSS_INDEX_COLUMN_OPERATION]));

            if (operation.compare("training") == 0) {

                auto i = 0;
                for (auto iter = p->indexes.begin(); iter != p->indexes.end(); ++iter, i++) {

                    vec_ptr vec;
                    if ((vec = p->vector_api->xValueAsVector(
                             argv[2 + VSS_INDEX_COLUMN_VECTORS + i])) != nullptr) {

                        iter->trainings.reserve(
                            vec->size() +
                            distance(vec->begin(),
                                     vec->end())); // TODO: Isn't the + distance
                                                   // reserving twice as much
                                                   // capacity as we need?

                        iter->trainings.insert(
                            iter->trainings.end(), vec->begin(),
                            vec->end());

                        p->isTraining = true;
                    }
                }

            } else {

                return SQLITE_ERROR;
            }
        }

    } else {

        // TODO: Implement - UPDATE operation
        sqlite3_free(pVTab->zErrMsg);

        pVTab->zErrMsg =
            sqlite3_mprintf("UPDATE on vss0 virtual tables not supported yet.");

        return SQLITE_ERROR;
    }

    return SQLITE_OK;
}

static void vssSearchFunc(sqlite3_context *context, int argc,
                          sqlite3_value **argv) { }

static void faissMemoryUsageFunc(sqlite3_context *context, int argc,
                                 sqlite3_value **argv) {

    sqlite3_result_int64(context, faiss::get_mem_usage_kb());
}

static void vssRangeSearchFunc(sqlite3_context *context, int argc,
                               sqlite3_value **argv) { }

static int vssIndexFindFunction(
                    sqlite3_vtab *pVtab,
                    int nArg,
                    const char *zName,
                     void (**pxFunc)(sqlite3_context *, int, sqlite3_value **),
                     void **ppArg) {

    if (sqlite3_stricmp(zName, "vss_search") == 0) {
        *pxFunc = vssSearchFunc;
        *ppArg = 0;
        return VSS_SEARCH_FUNCTION;
    }

    if (sqlite3_stricmp(zName, "vss_range_search") == 0) {
        *pxFunc = vssRangeSearchFunc;
        *ppArg = 0;
        return VSS_RANGE_SEARCH_FUNCTION;
    }
    return 0;
};

static int vssIndexShadowName(const char *zName) {

    static const char *azName[] = {"index", "data"};

    for (auto i = 0; i < sizeof(azName) / sizeof(azName[0]); i++) {
        if (sqlite3_stricmp(zName, azName[i]) == 0)
            return 1;
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
    /* xShadowName */ vssIndexShadowName};

#pragma endregion

#pragma region entrypoint

vector0_api *vector0_api_from_db(sqlite3 *db) {

    vector0_api *pRet = 0;
    sqlite3_stmt *pStmt = 0;

    if (SQLITE_OK == sqlite3_prepare(db, "SELECT vector0(?1)", -1, &pStmt, 0)) {

        sqlite3_bind_pointer(pStmt, 1, (void *)&pRet, "vector0_api_ptr", nullptr);
        sqlite3_step(pStmt);
    }

    sqlite3_finalize(pStmt);
    return pRet;
}

extern "C" {
#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_vss_init(sqlite3 *db, char **pzErrMsg,
                         const sqlite3_api_routines *pApi) {

        SQLITE_EXTENSION_INIT2(pApi);

        vector0_api *vector_api = vector0_api_from_db(db);

        if (vector_api == nullptr) {

            *pzErrMsg = sqlite3_mprintf(
                "The vector0 extension must be registered before vss0.");
            return SQLITE_ERROR;
        }

        sqlite3_create_function_v2(db, "vss_version", 0,
                                  SQLITE_UTF8 | SQLITE_DETERMINISTIC |
                                      SQLITE_INNOCUOUS,
                                  0, vss_version, 0, 0, 0);

        sqlite3_create_function_v2(db, "vss_debug", 0,
                                  SQLITE_UTF8 | SQLITE_DETERMINISTIC |
                                      SQLITE_INNOCUOUS,
                                  0, vss_debug, 0, 0, 0);

        sqlite3_create_function_v2(db, "vss_distance_l1", 2,
                                  SQLITE_UTF8 | SQLITE_DETERMINISTIC |
                                      SQLITE_INNOCUOUS,
                                  vector_api, vss_distance_l1, 0, 0, 0);

        sqlite3_create_function_v2(db, "vss_distance_l2", 2,
                                  SQLITE_UTF8 | SQLITE_DETERMINISTIC |
                                      SQLITE_INNOCUOUS,
                                  vector_api, vss_distance_l2, 0, 0, 0);

        sqlite3_create_function_v2(db, "vss_distance_linf", 2,
                                  SQLITE_UTF8 | SQLITE_DETERMINISTIC |
                                      SQLITE_INNOCUOUS,
                                  vector_api, vss_distance_linf, 0, 0, 0);

        sqlite3_create_function_v2(db, "vss_inner_product", 2,
                                  SQLITE_UTF8 | SQLITE_DETERMINISTIC |
                                      SQLITE_INNOCUOUS,
                                  vector_api, vss_inner_product, 0, 0, 0);

        sqlite3_create_function_v2(db, "vss_fvec_add", 2,
                                  SQLITE_UTF8 | SQLITE_DETERMINISTIC |
                                      SQLITE_INNOCUOUS,
                                  vector_api, vss_fvec_add, 0, 0, 0);

        sqlite3_create_function_v2(db, "vss_fvec_sub", 2,
                                  SQLITE_UTF8 | SQLITE_DETERMINISTIC |
                                      SQLITE_INNOCUOUS,
                                  vector_api, vss_fvec_sub, 0, 0, 0);

        sqlite3_create_function_v2(db, "vss_search", 2,
                                  SQLITE_UTF8 | SQLITE_DETERMINISTIC |
                                      SQLITE_INNOCUOUS,
                                  vector_api, vssSearchFunc, 0, 0, 0);

        sqlite3_create_function_v2(db, "vss_search_params", 2, 0, vector_api,
                                  VssSearchParamsFunc, 0, 0, 0);

        sqlite3_create_function_v2(db, "vss_range_search", 2,
                                  SQLITE_UTF8 | SQLITE_DETERMINISTIC |
                                      SQLITE_INNOCUOUS,
                                  vector_api, vssRangeSearchFunc, 0, 0, 0);

        sqlite3_create_function_v2(db, "vss_range_search_params", 2, 0, vector_api,
                                  VssRangeSearchParamsFunc, 0, 0, 0);

        sqlite3_create_module_v2(db, "vss0", &vssIndexModule, vector_api, 0);

        return 0;
    }
}

#pragma endregion
