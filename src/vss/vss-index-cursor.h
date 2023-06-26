
#ifndef VSS_INDEX_CURSOR_H
#define VSS_INDEX_CURSOR_H

#include "inclusions.h"

class vss_index_cursor : public sqlite3_vtab_cursor {

public:

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

    vss_index_vtab * getTable() {

        return table;
    }

    sqlite3_int64 getICurrent() {

        return iCurrent;
    }

    sqlite3_int64 getIRowid() {

        return iRowid;
    }

    QueryType getQuery_type() {

        return query_type;
    }

    sqlite3_int64 getLimit() {

        return limit;
    }

    vector<faiss::idx_t> & getSearch_ids() {

        return search_ids;
    }

    vector<float> & getSearch_distances() {

        return search_distances;
    }

    unique_ptr<faiss::RangeSearchResult> & getRange_search_result() {

        return range_search_result;
    }

    sqlite3_stmt *getStmt() {

        return stmt;
    }

    int getStep_result() {

        return step_result;
    }

    void setStep_result(int value) {

        step_result = value;
    }

    void incrementICurrent() {

        iCurrent += 1;
    }

    void setICurrent(sqlite3_int64 value) {

        iCurrent = value;
    }

    void resetSearch(long noItems) {

        search_distances = vector<float>(noItems, 0);
        search_ids = vector<faiss::idx_t>(noItems, 0);
    }

    void setQuery_type(QueryType value) {

        query_type = value;
    }

    void setSql(char * value) {

        if (sql != nullptr)
            sqlite3_free(sql);
        sql = value;
    }

    char * getSql() {

        return sql;
    }

    void setLimit(sqlite3_int64 value) {

        limit = value;
    }

    // TODO: Parts of our logic requires the address to the pointer such that we can assign what it's pointing at
    sqlite3_stmt *stmt;

private:

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
    char *sql;
    int step_result;
};

#endif // VSS_INDEX_CURSOR_H
