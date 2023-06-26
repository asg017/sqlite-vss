
#include "sqlite-vss.h"
#include "vss/inclusions.h"

#include "sqlite-vector.h"
#include "vss/sql-statement.h"
#include "vss/meta-methods.h"
#include "vss/calculations.h"
#include "vss/vss-index.h"
#include "vss/vss-index-vtab.h"
#include "vss/vss-index-cursor.h"

#pragma region Structs and cleanup functions

struct VssSearchParams {

    vec_ptr vector;
    sqlite3_int64 k;
};

void delVssSearchParams(void *p) {

    VssSearchParams *self = (VssSearchParams *)p;
    delete self;
}

struct VssRangeSearchParams {

    vec_ptr vector;
    float distance;
};

void delVssRangeSearchParams(void *p) {

    auto self = (VssRangeSearchParams *)p;
    delete self;
}

#pragma endregion

#pragma region Vtab

static void vssSearchParamsFunc(sqlite3_context *context,
                                int argc,
                                sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr vector = vector_api->xValueAsVector(argv[0]);
    if (vector == nullptr) {
        sqlite3_result_error(context, "1st argument is not a vector", -1);
        return;
    }

    auto limit = sqlite3_value_int64(argv[1]);
    auto params = new VssSearchParams();
    params->vector = vec_ptr(vector.release());
    params->k = limit;
    sqlite3_result_pointer(context, params, "vss0_searchparams", delVssSearchParams);
}

static void vssRangeSearchParamsFunc(sqlite3_context *context,
                                     int argc,
                                     sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr vector = vector_api->xValueAsVector(argv[0]);
    if (vector == nullptr) {
        sqlite3_result_error(context, "1st argument is not a vector", -1);
        return;
    }

    auto params = new VssRangeSearchParams();

    params->vector = vec_ptr(vector.release());
    params->distance = sqlite3_value_double(argv[1]);

    sqlite3_result_pointer(context, params, "vss0_rangesearchparams", delVssRangeSearchParams);
}

static int write_index_insert(faiss::VectorIOWriter &writer,
                              sqlite3 *db,
                              char *schema,
                              char *name,
                              int rowId) {

    // If inserts fails it means index already exists.
    SqlStatement insert(db,
                        sqlite3_mprintf("insert into \"%w\".\"%w_index\"(rowid, idx) values (?, ?)",
                                        schema,
                                        name));

    if (insert.prepare() != SQLITE_OK)
        return SQLITE_ERROR;

    if (insert.bind_int64(1, rowId) != SQLITE_OK)
        return SQLITE_ERROR;

    if (insert.bind_blob64(2, writer.data.data(), writer.data.size()) != SQLITE_OK)
        return SQLITE_ERROR;

    auto rc = insert.step();
    if (rc == SQLITE_DONE)
        return SQLITE_OK; // Index did not exist, and we successfully inserted it.

    return rc;
}

static int write_index_update(faiss::VectorIOWriter &writer,
                              sqlite3 *db,
                              char *schema,
                              char *name,
                              int rowId) {

    // Updating existing index.
    SqlStatement update(db,
                        sqlite3_mprintf("update \"%w\".\"%w_index\" set idx = ? where rowid = ?",
                                        schema,
                                        name));

    if (update.prepare() != SQLITE_OK)
        return SQLITE_ERROR;

    if (update.bind_blob64(1, writer.data.data(), writer.data.size()) != SQLITE_OK)
        return SQLITE_ERROR;

    if (update.bind_int64(2, rowId) != SQLITE_OK)
        return SQLITE_ERROR;

    auto rc = update.step();
    if (rc == SQLITE_DONE)
        return SQLITE_OK; // We successfully updated existing index.

    return rc;
}

static int write_index(faiss::Index *index,
                       sqlite3 *db,
                       char *schema,
                       char *name,
                       int rowId) {

    // Writing our index
    faiss::VectorIOWriter writer;
    faiss::write_index(index, &writer);

    // First trying to insert index, if that fails with ROW constraing error, we try to update existing index.
    if (write_index_insert(writer, db, schema, name, rowId) == SQLITE_OK)
        return SQLITE_OK;

    if (sqlite3_extended_errcode(db) != SQLITE_CONSTRAINT_ROWID)
        return SQLITE_ERROR; // Insert failed for unknown error

    // Insert failed because index already existed, updating existing index.
    return write_index_update(writer, db, schema, name, rowId);
}

static int shadow_data_insert(sqlite3 *db,
                              char *schema,
                              char *name,
                              sqlite3_int64 rowid) {

    SqlStatement insert(db,
                        sqlite3_mprintf("insert into \"%w\".\"%w_data\"(rowid, x) values (?, ?);",
                                        schema,
                                        name));

    if (insert.prepare() != SQLITE_OK)
        return SQLITE_ERROR;

    if (insert.bind_int64(1, rowid) != SQLITE_OK)
        return SQLITE_ERROR;

    if (insert.bind_null(2) != SQLITE_OK)
        return SQLITE_ERROR;

    if (insert.step() != SQLITE_DONE)
        return SQLITE_ERROR;

    return SQLITE_OK;
}

static int shadow_data_delete(sqlite3 *db,
                              char *schema,
                              char *name,
                              sqlite3_int64 rowid) {

    SqlStatement del(db,
                     sqlite3_mprintf("delete from \"%w\".\"%w_data\" where rowid = ?",
                                     schema,
                                     name));

    if (del.prepare() != SQLITE_OK)
        return SQLITE_ERROR;

    if (del.bind_int64(1, rowid) != SQLITE_OK)
        return SQLITE_ERROR;

    if (del.step() != SQLITE_DONE)
        return SQLITE_ERROR;

    return SQLITE_OK;
}

static faiss::Index *read_index_select(sqlite3 *db,
                                       const char *name,
                                       int indexId) {

    SqlStatement select(db,
                        sqlite3_mprintf("select idx from \"%w_index\" where rowid = ?",
                                        name));

    if (select.prepare() != SQLITE_OK)
        return nullptr;

    if (select.bind_int64(1, indexId) != SQLITE_OK)
        return nullptr;

    if (select.step() != SQLITE_ROW)
        return nullptr;

    auto index_data = select.column_blob(0);
    auto size = select.column_bytes(0);

    faiss::VectorIOReader reader;
    copy((const uint8_t *)index_data,
         ((const uint8_t *)index_data) + size,
         back_inserter(reader.data));

    return faiss::read_index(&reader);
}

static int create_shadow_tables(sqlite3 *db,
                                const char *schema,
                                const char *name,
                                int n) {

    SqlStatement create1(db,
                         sqlite3_mprintf("create table \"%w\".\"%w_index\"(idx)",
                                         schema,
                                         name));

    auto rc = create1.exec();
    if (rc != SQLITE_OK)
        return rc;

    /*
     * Notice, we'll need to explicitly finalize this object since we can only
     * have one open statement at the same time to the same connetion.
     */
    create1.finalize();

    SqlStatement create2(db,
                         sqlite3_mprintf("create table \"%w\".\"%w_data\"(x);",
                                         schema,
                                         name));

    rc = create2.exec();
    return rc;
}

static int drop_shadow_tables(sqlite3 *db, char *name) {

    const char *drops[2] = {"drop table \"%w_index\";",
                            "drop table \"%w_data\";"};

    for (int i = 0; i < 2; i++) {

        SqlStatement cur(db,
                         sqlite3_mprintf(drops[i],
                                         name));

        if (cur.prepare() != SQLITE_OK)
            return SQLITE_ERROR;

        if (cur.step() != SQLITE_DONE)
            return SQLITE_ERROR;
    }
    return SQLITE_OK;
}

struct VssIndexColumn {

    string name;
    sqlite3_int64 dimensions;
    string factory;
};

unique_ptr<vector<VssIndexColumn>> parse_constructor(int argc,
                                                     const char *const *argv) {

    auto columns = unique_ptr<vector<VssIndexColumn>>(new vector<VssIndexColumn>());

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
                return nullptr;
            }
            factory = arg.substr(lquote + 1, rquote - lquote - 1);

        } else {
            factory = string("Flat,IDMap2");
        }
        columns->push_back(VssIndexColumn{name, dimensions, factory});
    }

    return columns;
}

#define VSS_INDEX_COLUMN_DISTANCE 0
#define VSS_INDEX_COLUMN_OPERATION 1
#define VSS_INDEX_COLUMN_VECTORS 2

static int init(sqlite3 *db,
                void *pAux,
                int argc,
                const char *const *argv,
                sqlite3_vtab **ppVtab,
                char **pzErr,
                bool isCreate) {

    sqlite3_vtab_config(db, SQLITE_VTAB_CONSTRAINT_SUPPORT, 1);

    auto columns = parse_constructor(argc, argv);
    if (columns == nullptr) {
        *pzErr = sqlite3_mprintf("Error parsing VSS index factory constructor");
        return SQLITE_ERROR;
    }

    string sql = "create table x(distance hidden, operation hidden";
    for (auto colIter = columns->begin(); colIter != columns->end(); ++colIter) {
        sql += ", \"" + colIter->name + "\"";
    }
    sql += ")";

    SqlStatement create(db,
                        sqlite3_mprintf(sql.c_str()));

    auto rc = create.declare_vtab();

    if (rc != SQLITE_OK)
        return rc;

    auto pTable = new vss_index_vtab(db,
                                     (vector0_api *)pAux,
                                     sqlite3_mprintf("%s", argv[1]),
                                     sqlite3_mprintf("%s", argv[2]));

    *ppVtab = pTable;

    if (isCreate) {

        for (auto iter = columns->begin(); iter != columns->end(); ++iter) {

            try {

                auto index = faiss::index_factory(iter->dimensions, iter->factory.c_str());
                pTable->getIndexes().push_back(new vss_index(index));

            } catch (faiss::FaissException &e) {

                *pzErr = sqlite3_mprintf("Error building index factory for %s, exception was: %s",
                                         iter->name.c_str(),
                                         e.msg.c_str());

                return SQLITE_ERROR;
            }
        }

        rc = create_shadow_tables(db, argv[1], argv[2], columns->size());
        if (rc != SQLITE_OK)
            return rc;

        // Shadow tables were successully created.
        // After shadow tables are created, write the initial index state to
        // shadow _index.
        auto i = 0;
        for (auto iter = pTable->getIndexes().begin(); iter != pTable->getIndexes().end(); ++iter, i++) {

            try {

                int rc = write_index((*iter)->getIndex(),
                                            pTable->getDb(),
                                            pTable->getSchema(),
                                            pTable->getName(),
                                            i);

                if (rc != SQLITE_OK)
                    return rc;

            } catch (faiss::FaissException &e) {

                return SQLITE_ERROR;
            }
        }

    } else {

        for (int i = 0; i < columns->size(); i++) {

            auto index = read_index_select(db, argv[2], i);

            // Index in shadow table should always be available, integrity check
            // to avoid null pointer
            if (index == nullptr) {
                *pzErr = sqlite3_mprintf("Could not read index at position %d", i);
                return SQLITE_ERROR;
            }
            pTable->getIndexes().push_back(new vss_index(index));
        }
    }

    return SQLITE_OK;
}

static int vssIndexCreate(sqlite3 *db, void *pAux,
                          int argc,
                          const char *const *argv,
                          sqlite3_vtab **ppVtab,
                          char **pzErr) {

    return init(db, pAux, argc, argv, ppVtab, pzErr, true);
}

static int vssIndexConnect(sqlite3 *db,
                           void *pAux, int argc,
                           const char *const *argv,
                           sqlite3_vtab **ppVtab,
                           char **pzErr) {

    return init(db, pAux, argc, argv, ppVtab, pzErr, false);
}

static int vssIndexDisconnect(sqlite3_vtab *pVtab) {

    auto pTable = static_cast<vss_index_vtab *>(pVtab);
    delete pTable;
    return SQLITE_OK;
}

static int vssIndexDestroy(sqlite3_vtab *pVtab) {

    auto pTable = static_cast<vss_index_vtab *>(pVtab);
    drop_shadow_tables(pTable->getDb(), pTable->getName());
    vssIndexDisconnect(pVtab);
    return SQLITE_OK;
}

static int vssIndexOpen(sqlite3_vtab *pVtab, sqlite3_vtab_cursor **ppCursor) {

    auto pTable = static_cast<vss_index_vtab *>(pVtab);

    auto pCursor = new vss_index_cursor(pTable);
    if (pCursor == nullptr)
        return SQLITE_NOMEM;

    *ppCursor = pCursor;

    return SQLITE_OK;
}

static int vssIndexClose(sqlite3_vtab_cursor *cur) {

    auto pCursor = static_cast<vss_index_cursor *>(cur);
    delete pCursor;
    return SQLITE_OK;
}

static int vssIndexBestIndex(sqlite3_vtab *tab, sqlite3_index_info *pIdxInfo) {

    int iSearchTerm = -1;
    int iRangeSearchTerm = -1;
    int iXSearchColumn = -1;
    int iLimit = -1;

    for (int i = 0; i < pIdxInfo->nConstraint; i++) {

        auto constraint = pIdxInfo->aConstraint[i];

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

static int vssIndexFilter(sqlite3_vtab_cursor *pVtabCursor,
                          int idxNum,
                          const char *idxStr,
                          int argc,
                          sqlite3_value **argv) {

    auto pCursor = static_cast<vss_index_cursor *>(pVtabCursor);

    if (strcmp(idxStr, "search") == 0) {

        pCursor->setQuery_type(QueryType::search);
        vec_ptr query_vector;

        auto params = static_cast<VssSearchParams *>(sqlite3_value_pointer(argv[0], "vss0_searchparams"));
        if (params != nullptr) {

            pCursor->setLimit(params->k);
            query_vector = vec_ptr(new vector<float>(*params->vector));

        } else if (sqlite3_libversion_number() < 3041000) {

            // https://sqlite.org/forum/info/6b32f818ba1d97ef
            auto ptrVtab = static_cast<vss_index_vtab *>(pCursor->pVtab);
            ptrVtab->setError(
                sqlite3_mprintf(
                    "vss_search() only support vss_search_params() as a "
                    "2nd parameter for SQLite versions below 3.41.0"));
            return SQLITE_ERROR;

        } else if ((query_vector = pCursor->getTable()->getVector0_api()->xValueAsVector(
                        argv[0])) != nullptr) {

            if (argc > 1) {

                pCursor->setLimit(sqlite3_value_int(argv[1]));
            } else {

                auto ptrVtab = static_cast<vss_index_vtab *>(pCursor->pVtab);
                ptrVtab->setError(sqlite3_mprintf("LIMIT required on vss_search() queries"));

                return SQLITE_ERROR;
            }

        } else {

            auto ptrVtab = static_cast<vss_index_vtab *>(pCursor->pVtab);
            ptrVtab->setError(sqlite3_mprintf("2nd argument to vss_search() must be a vector"));

            return SQLITE_ERROR;
        }

        int nq = 1;
        auto index = pCursor->getTable()->getIndexes().at(idxNum)->getIndex();

        if (query_vector->size() != index->d) {

            auto ptrVtab = static_cast<vss_index_vtab *>(pCursor->pVtab);
            ptrVtab->setError(sqlite3_mprintf(
                "Input query size doesn't match index dimensions: %ld != %ld",
                query_vector->size(),
                index->d));

            return SQLITE_ERROR;
        }

        if (pCursor->getLimit() <= 0) {

            auto ptrVtab = static_cast<vss_index_vtab *>(pCursor->pVtab);
            ptrVtab->setError(sqlite3_mprintf(
                "Limit must be greater than 0, got %ld",
                pCursor->getLimit()));

            return SQLITE_ERROR;
        }

        // To avoid trying to select more records than number of records in index.
        auto searchMax = min(static_cast<faiss::idx_t>(pCursor->getLimit()) * nq, index->ntotal * nq);

        pCursor->resetSearch(searchMax);

        index->search(nq,
                      query_vector->data(),
                      searchMax,
                      pCursor->getSearch_distances().data(),
                      pCursor->getSearch_ids().data());

    } else if (strcmp(idxStr, "range_search") == 0) {

        pCursor->setQuery_type(QueryType::range_search);

        auto params = static_cast<VssRangeSearchParams *>(
            sqlite3_value_pointer(argv[0], "vss0_rangesearchparams"));

        int nq = 1;

        vector<faiss::idx_t> nns(params->distance * nq);
        pCursor->getRange_search_result() = unique_ptr<faiss::RangeSearchResult>(new faiss::RangeSearchResult(nq, true));

        auto index = pCursor->getTable()->getIndexes().at(idxNum)->getIndex();

        index->range_search(nq,
                            params->vector->data(),
                            params->distance,
                            pCursor->getRange_search_result().get());

    } else if (strcmp(idxStr, "fullscan") == 0) {

        pCursor->setQuery_type(QueryType::fullscan);
        pCursor->setSql(
                        sqlite3_mprintf("select rowid from \"%w_data\"",
                                        pCursor->getTable()->getName()));

        int res = sqlite3_prepare_v2(pCursor->getTable()->getDb(),
                                     pCursor->getSql(),
                                     -1,
                                     &pCursor->stmt,
                                     nullptr);

        if (res != SQLITE_OK)
            return res;

        pCursor->setStep_result(sqlite3_step(pCursor->getStmt()));

    } else {

        auto ptrVtab = static_cast<vss_index_vtab *>(pCursor->pVtab);
        ptrVtab->setError(sqlite3_mprintf(
            "%s %s", "vssIndexFilter error: unhandled idxStr", idxStr));

        return SQLITE_ERROR;
    }

    pCursor->setICurrent(0);
    return SQLITE_OK;
}

static int vssIndexNext(sqlite3_vtab_cursor *cur) {

    auto pCursor = static_cast<vss_index_cursor *>(cur);

    switch (pCursor->getQuery_type()) {

      case QueryType::search:
      case QueryType::range_search:
          pCursor->incrementICurrent();
          break;

      case QueryType::fullscan:
          pCursor->setStep_result(sqlite3_step(pCursor->getStmt()));
    }

    return SQLITE_OK;
}

static int vssIndexRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid) {

    auto pCursor = static_cast<vss_index_cursor *>(cur);

    switch (pCursor->getQuery_type()) {

        case QueryType::search:
            *pRowid = pCursor->getSearch_ids().at(pCursor->getICurrent());
            break;

        case QueryType::range_search:
            *pRowid = pCursor->getRange_search_result()->labels[pCursor->getICurrent()];
            break;

        case QueryType::fullscan:
            *pRowid = sqlite3_column_int64(pCursor->getStmt(), 0);
            break;
    }
    return SQLITE_OK;
}

static int vssIndexEof(sqlite3_vtab_cursor *cur) {

    auto pCursor = static_cast<vss_index_cursor *>(cur);

    switch (pCursor->getQuery_type()) {

      case QueryType::search:
          return pCursor->getICurrent() >= pCursor->getLimit() ||
                pCursor->getICurrent() >= pCursor->getSearch_ids().size()
                || (pCursor->getSearch_ids().at(pCursor->getICurrent()) == -1);

      case QueryType::range_search:
          return pCursor->getICurrent() >= pCursor->getRange_search_result()->lims[1];

      case QueryType::fullscan:
          return pCursor->getStep_result() != SQLITE_ROW;
    }
    return 1;
}

static int vssIndexColumn(sqlite3_vtab_cursor *cur,
                          sqlite3_context *ctx,
                          int i) {

    auto pCursor = static_cast<vss_index_cursor *>(cur);

    if (i == VSS_INDEX_COLUMN_DISTANCE) {

        switch (pCursor->getQuery_type()) {

          case QueryType::search:
              sqlite3_result_double(ctx,
                                    pCursor->getSearch_distances().at(pCursor->getICurrent()));
              break;

          case QueryType::range_search:
              sqlite3_result_double(ctx,
                                    pCursor->getRange_search_result()->distances[pCursor->getICurrent()]);
              break;

          case QueryType::fullscan:
              break;
        }

    } else if (i >= VSS_INDEX_COLUMN_VECTORS) {

        auto index =
            pCursor->getTable()->getIndexes().at(i - VSS_INDEX_COLUMN_VECTORS)->getIndex();

        vector<float> vec(index->d);
        sqlite3_int64 rowId;
        vssIndexRowid(cur, &rowId);

        try {

            index->reconstruct(rowId, vec.data());

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
        pCursor->getTable()->getVector0_api()->xResultVector(ctx, &vec);
    }
    return SQLITE_OK;
}

static int vssIndexBegin(sqlite3_vtab *tab) {

    return SQLITE_OK;
}

static int vssIndexSync(sqlite3_vtab *pVTab) {

    auto pTable = static_cast<vss_index_vtab *>(pVTab);

    try {

        auto i = 0;
        for (auto iter = pTable->getIndexes().begin(); iter != pTable->getIndexes().end(); ++iter, i++) {

            // Synchronizing index, implying deleting, training, and inserting records according to needs.
            if ((*iter)->synchronize()) {

                /*
                 * If the above invocation returned true, we've got updates to currently iterated index,
                 * hence writing to db.
                 */
                int rc = write_index((*iter)->getIndex(),
                                     pTable->getDb(),
                                     pTable->getSchema(),
                                     pTable->getName(),
                                     i);

                if (rc != SQLITE_OK) {

                    pTable->setError(sqlite3_mprintf("Error saving index (%d): %s",
                                                     rc,
                                                     sqlite3_errmsg(pTable->getDb())));

                    // Clearing all indexes to cleanup after ourselves.
                    for (auto iter2 = pTable->getIndexes().begin(); iter2 != pTable->getIndexes().end(); ++iter2) {

                        (*iter2)->reset();
                    }
                    return rc;
                }
            }
        }

        return SQLITE_OK;

    } catch (faiss::FaissException &e) {

        pTable->setError(sqlite3_mprintf("Error during synchroning index. Full error: %s",
                                         e.msg.c_str()));

        for (auto iter = pTable->getIndexes().begin(); iter != pTable->getIndexes().end(); ++iter) {

            // Cleanups in case we've got hanging data.
            (*iter)->reset();
        }

        return SQLITE_ERROR;
    }
}

static int vssIndexCommit(sqlite3_vtab *pVTab) { return SQLITE_OK; }

static int vssIndexRollback(sqlite3_vtab *pVTab) {

    auto pTable = static_cast<vss_index_vtab *>(pVTab);

    for (auto iter = pTable->getIndexes().begin(); iter != pTable->getIndexes().end(); ++iter) {

        (*iter)->reset();
    }
    return SQLITE_OK;
}

static int vssIndexUpdate(sqlite3_vtab *pVTab,
                          int argc,
                          sqlite3_value **argv,
                          sqlite_int64 *pRowid) {

    auto pTable = static_cast<vss_index_vtab *>(pVTab);

    if (argc == 1 && sqlite3_value_type(argv[0]) != SQLITE_NULL) {

        // DELETE operation
        sqlite3_int64 rowid_to_delete = sqlite3_value_int64(argv[0]);

        auto rc = shadow_data_delete(pTable->getDb(),
                                     pTable->getSchema(),
                                     pTable->getName(),
                                     rowid_to_delete);
        if (rc != SQLITE_OK)
            return rc;

        for (auto iter = pTable->getIndexes().begin(); iter != pTable->getIndexes().end(); ++iter) {
            (*iter)->addDelete(rowid_to_delete);
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
            for (auto iter = pTable->getIndexes().begin(); iter != pTable->getIndexes().end(); ++iter, i++) {

                if ((vec = pTable->getVector0_api()->xValueAsVector(
                         argv[2 + VSS_INDEX_COLUMN_VECTORS + i])) != nullptr) {

                    // Make sure the index is already trained, if it's needed
                    if (!(*iter)->getIndex()->is_trained) {

                        pTable->setError(sqlite3_mprintf("Index at i=%d requires training "
                                                         "before inserting data.",
                                                         i));

                        return SQLITE_ERROR;
                    }

                    if (!inserted_rowid) {

                        auto rc = shadow_data_insert(pTable->getDb(),
                                                     pTable->getSchema(),
                                                     pTable->getName(),
                                                     rowid);

                        if (rc != SQLITE_OK)
                            return rc;

                        inserted_rowid = true;
                    }

                    (*iter)->addInsertData(rowid, vec);
                    *pRowid = rowid;
                }
            }

        } else {

            string operation((char *)sqlite3_value_text(argv[2 + VSS_INDEX_COLUMN_OPERATION]));

            if (operation.compare("training") == 0) {

                auto i = 0;
                for (auto iter = pTable->getIndexes().begin(); iter != pTable->getIndexes().end(); ++iter, i++) {

                    vec_ptr vec = pTable->getVector0_api()->xValueAsVector(argv[2 + VSS_INDEX_COLUMN_VECTORS + i]);
                    if (vec != nullptr)
                        (*iter)->addTrainings(vec);
                }

            } else {

                return SQLITE_ERROR;
            }
        }

    } else {

        // TODO: Implement - UPDATE operation
        pTable->setError(sqlite3_mprintf("UPDATE statements on vss0 virtual tables not supported yet."));
        return SQLITE_ERROR;
    }

    return SQLITE_OK;
}

static void vssSearchFunc(sqlite3_context *context,
                          int argc,
                          sqlite3_value **argv) { }

static void faissMemoryUsageFunc(sqlite3_context *context,
                                 int argc,
                                 sqlite3_value **argv) {

    sqlite3_result_int64(context, faiss::get_mem_usage_kb());
}

static void vssRangeSearchFunc(sqlite3_context *context,
                               int argc,
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
    /* iVersion    */ 3, // TODO: Shouldn't this be the same as the version for sqlite-vector.cpp?
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
    /* xRename     */ nullptr,
    /* xSavepoint  */ nullptr,
    /* xRelease    */ nullptr,
    /* xRollbackTo */ nullptr,
    /* xShadowName */ vssIndexShadowName
};

#pragma endregion

#pragma region entrypoint

vector0_api *vector0_api_from_db(sqlite3 *db) {

    SqlStatement select(db, sqlite3_mprintf("select vector0(?1)"));
    if (select.prepare() != SQLITE_OK)
        return nullptr;

    vector0_api *pRet = nullptr;
    if (select.bind_pointer(1, (void *)&pRet, "vector0_api_ptr") != SQLITE_OK)
        return nullptr;

    if (select.step() != SQLITE_ROW)
        return nullptr;

    return pRet;
}

extern "C" {
#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_vss_init(sqlite3 *db,
                         char **pzErrMsg,
                         const sqlite3_api_routines *pApi) {

        SQLITE_EXTENSION_INIT2(pApi);

        auto vector_api = vector0_api_from_db(db);

        if (vector_api == nullptr) {

            *pzErrMsg = sqlite3_mprintf(
                "The vector0 extension must be registered before vss0.");
            return SQLITE_ERROR;
        }

        // TODO: This should preferably be done the same way it's done in sqlite-vector.cpp by using an array.
        sqlite3_create_function_v2(db,
                                   "vss_version",
                                   0,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   0,
                                   vss_version,
                                   nullptr, nullptr, nullptr);

        sqlite3_create_function_v2(db,
                                   "vss_debug",
                                   0,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   0,
                                   vss_debug,
                                   nullptr, nullptr, nullptr);

        sqlite3_create_function_v2(db,
                                   "vss_distance_l1",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_distance_l1,
                                   nullptr, nullptr, nullptr);

        sqlite3_create_function_v2(db, "vss_distance_l2",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_distance_l2,
                                   nullptr, nullptr, nullptr);

        sqlite3_create_function_v2(db, "vss_distance_linf",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_distance_linf,
                                   nullptr, nullptr, nullptr);

        sqlite3_create_function_v2(db, "vss_inner_product",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_inner_product,
                                   nullptr, nullptr, nullptr);

        sqlite3_create_function_v2(db, "vss_fvec_add",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_fvec_add,
                                   nullptr, nullptr, nullptr);

        sqlite3_create_function_v2(db, "vss_fvec_sub",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_fvec_sub,
                                   nullptr, nullptr, nullptr);

        sqlite3_create_function_v2(db, "vss_search",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vssSearchFunc,
                                   nullptr, nullptr, nullptr);

        sqlite3_create_function_v2(db,
                                   "vss_search_params",
                                   2,
                                   0,
                                   vector_api,
                                   vssSearchParamsFunc,
                                   nullptr, nullptr, nullptr);

        sqlite3_create_function_v2(db,
                                   "vss_range_search",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vssRangeSearchFunc,
                                   nullptr, nullptr, nullptr);

        sqlite3_create_function_v2(db,
                                   "vss_range_search_params",
                                   2,
                                   0,
                                   vector_api,
                                   vssRangeSearchParamsFunc,
                                   nullptr, nullptr, nullptr);

        sqlite3_create_function_v2(db,
                                   "vss_memory_usage",
                                   0,
                                   0,
                                   nullptr,
                                   faissMemoryUsageFunc,
                                   nullptr, nullptr, nullptr);

        auto rc = sqlite3_create_module_v2(db, "vss0", &vssIndexModule, vector_api, nullptr);
        if (rc != SQLITE_OK) {

            *pzErrMsg = sqlite3_mprintf("%s", sqlite3_errmsg(db));
            return rc;
        }

        return 0;
    }
}

#pragma endregion
