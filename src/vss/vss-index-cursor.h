
#ifndef VSS_INDEX_CURSOR_H
#define VSS_INDEX_CURSOR_H

#include "inclusions.h"

struct vss_index_cursor : public sqlite3_vtab_cursor {

    explicit vss_index_cursor(vss_index_vtab *table)
      : table(table),
        sqlite3_vtab_cursor({0}),
        stmt(nullptr),
        sql(nullptr) { }

    ~vss_index_cursor() {
        if (stmt != nullptr)
            sqlite3_finalize(stmt);
        if (sql != nullptr)
            sqlite3_free(sql);
    }

    vss_index_vtab *table;

    sqlite3_int64 iCurrent;
    sqlite3_int64 iRowid;

    QueryType query_type;

    // For query_type == QueryType::search
    sqlite3_int64 limit;
    vector<faiss::idx_t> search_ids;
    vector<float> search_distances;

    // For query_type == QueryType::range_search
    unique_ptr<faiss::RangeSearchResult> range_search_result;

    // For query_type == QueryType::fullscan
    sqlite3_stmt *stmt;
    char *sql;
    int step_result;
};

#endif // VSS_INDEX_CURSOR_H
