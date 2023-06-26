
#ifndef SQL_STATEMENT_H
#define SQL_STATEMENT_H

#include "sqlite-vss.h"

class SqlStatement {

public:

    SqlStatement(sqlite3 *db, const char * sql) : db(db), sql(sql), stmt(nullptr) {

        this->sql = sql;
    }

    ~SqlStatement() {

        if (stmt != nullptr)
            sqlite3_finalize(stmt);
        if (sql != nullptr)
            sqlite3_free((void *)sql);
    }

    int prepare() {

        auto res = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (res != SQLITE_OK || stmt == nullptr) {

            stmt = nullptr;
            return SQLITE_ERROR;
        }
        return res;
    }

    int bind_int64(int colNo, sqlite3_int64 value) {

        return sqlite3_bind_int64(stmt, colNo, value);
    }

    int bind_blob64(int colNo, const void * data, int size) {

        return sqlite3_bind_blob64(stmt, colNo, data, size, SQLITE_TRANSIENT);
    }

    int bind_null(int colNo) {

        return sqlite3_bind_null(stmt, colNo);
    }

    int bind_pointer(int paramNo, void *ptr, const char * name) {

        return sqlite3_bind_pointer(stmt, paramNo, ptr, name, nullptr);
    }

    int step() {

        return sqlite3_step(stmt);
    }

    int exec() {

        return sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }

    int declare_vtab() {

        return sqlite3_declare_vtab(db, sql);
    }

    const void * column_blob(int colNo) {

        return sqlite3_column_blob(stmt, colNo);
    }

    int column_bytes(int colNo) {

        return sqlite3_column_bytes(stmt, colNo);
    }

    int column_int64(int colNo) {

        return sqlite3_column_int64(stmt, colNo);
    }

    int last_insert_rowid() {

        return sqlite3_last_insert_rowid(db);
    }

    void finalize() {

        if (stmt != nullptr)
            sqlite3_finalize(stmt);
        stmt = nullptr;
        if (sql != nullptr)
            sqlite3_free((void *)sql);
        sql = nullptr;
    }

private:

    sqlite3 *db;
    sqlite3_stmt *stmt;
    const char * sql;
};

#endif // SQL_STATEMENT_H

