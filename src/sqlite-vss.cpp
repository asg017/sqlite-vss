#include "sqlite-vss.h"
#include <cstdio>
#include <cstdlib>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <fstream>
#include <functional>
#include <optional>

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

#pragma region Meta

static void vss_version(sqlite3_context *context, int argc,
                        sqlite3_value **argv) {

    sqlite3_result_text(context, SQLITE_VSS_VERSION, -1, SQLITE_STATIC);
}

static void vss_debug(sqlite3_context *context,
                      int argc,
                      sqlite3_value **argv) {

    auto resTxt = sqlite3_mprintf(
        "version: %s\nfaiss version: %d.%d.%d\nfaiss compile options: %s",
        SQLITE_VSS_VERSION,
        FAISS_VERSION_MAJOR,
        FAISS_VERSION_MINOR,
        FAISS_VERSION_PATCH,
        faiss::get_compile_options().c_str());

    sqlite3_result_text(context, resTxt, -1, SQLITE_TRANSIENT);
    sqlite3_free(resTxt);
}

#pragma endregion

#pragma region Distances

static void vss_distance_l1(sqlite3_context *context,
                            int argc,
                            sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size",
                             -1);
        return;
    }

    sqlite3_result_double(context, faiss::fvec_L1(lhs->data(), rhs->data(), lhs->size()));
}

static void vss_distance_l2(sqlite3_context *context, int argc,
                            sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size",
                             -1);
        return;
    }

    sqlite3_result_double(context, faiss::fvec_L2sqr(lhs->data(), rhs->data(), lhs->size()));
}

static void vss_distance_linf(sqlite3_context *context, int argc,
                              sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size",
                             -1);
        return;
    }

    sqlite3_result_double(context, faiss::fvec_Linf(lhs->data(), rhs->data(), lhs->size()));
}

static void vss_inner_product(sqlite3_context *context, int argc,
                              sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size",
                             -1);
        return;
    }

    sqlite3_result_double(context,
                          faiss::fvec_inner_product(lhs->data(), rhs->data(), lhs->size()));
}

static void vss_cosine_similarity(sqlite3_context *context, int argc,
                              sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size",
                             -1);
        return;
    }

    float inner_product = faiss::fvec_inner_product(lhs->data(), rhs->data(), lhs->size());
    float lhs_norm = faiss::fvec_norm_L2sqr(lhs->data(), lhs->size());
    float rhs_norm = faiss::fvec_norm_L2sqr(rhs->data(), rhs->size());

    if (lhs_norm == 0.0f || rhs_norm == 0.0f) {
        sqlite3_result_error(context, "One or both vectors are zero-vectors", -1);
        return;
    }

    sqlite3_result_double(context, inner_product / (sqrt(lhs_norm) * sqrt(rhs_norm)));
}

static void vss_fvec_add(sqlite3_context *context, int argc,
                         sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size",
                             -1);
        return;
    }

    auto size = lhs->size();
    vec_ptr c(new vector<float>(size));
    faiss::fvec_add(size, lhs->data(), rhs->data(), c->data());

    sqlite3_result_blob64(context, c->data(), c->size() * sizeof(float), SQLITE_TRANSIENT);
}

static void vss_fvec_sub(sqlite3_context *context, int argc,
                         sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size", -1);
        return;
    }

    int size = lhs->size();
    vec_ptr c = vec_ptr(new vector<float>(size));
    faiss::fvec_sub(size, lhs->data(), rhs->data(), c->data());
    sqlite3_result_blob64(context, c->data(), c->size() * sizeof(float), SQLITE_TRANSIENT);
}

#pragma endregion

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

// StorageType enum gives options for where to store faiss indices. Default is faiss_shadow.
// faiss_ondisk -> create files in the same directory as the database file for the indices.
enum StorageType { faiss_shadow, faiss_ondisk };

enum QueryType { search, range_search, fullscan };

// Wrapper around a single faiss index, with training data, insert records, and
// delete records.
struct vss_index {

    explicit vss_index(faiss::Index *index) : index(index) {}
    explicit vss_index(faiss::Index *index, string name, StorageType storage_type) : index(index), name(name), storage_type(storage_type) {}

    ~vss_index() {
        if (index != nullptr) {
            delete index;
        }
    }

    faiss::Index *index;
    vector<float> trainings;
    vector<float> insert_data;
    vector<faiss::idx_t> insert_ids;
    vector<faiss::idx_t> delete_ids;
    string name;
    StorageType storage_type;
};

struct vss_index_vtab : public sqlite3_vtab {

    vss_index_vtab(sqlite3 *db, vector0_api *vector_api, char *schema, char *name)
      : db(db),
        vector_api(vector_api),
        schema(schema),
        name(name) { }

    ~vss_index_vtab() {

        if (name)
            sqlite3_free(name);
        if (schema)
            sqlite3_free(schema);
        for (auto iter = indexes.begin(); iter != indexes.end(); ++iter) {
            delete (*iter);
        }
    }

    sqlite3 *db;
    vector0_api *vector_api;

    // Name of the virtual table. Must be freed during disconnect
    char *name;

    // Name of the schema the virtual table exists in. Must be freed during
    // disconnect
    char *schema;

    // Vector holding all the  faiss Indices the vtab uses, and their state,
    // implying which items are to be deleted and inserted.
    vector<vss_index*> indexes;
};

struct vss_index_cursor : public sqlite3_vtab_cursor {

    explicit vss_index_cursor(vss_index_vtab *table)
      : table(table),
        sqlite3_vtab_cursor({0}),
        stmt(nullptr) { }

    ~vss_index_cursor() {
        if (stmt != nullptr)
            sqlite3_finalize(stmt);
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
    int step_result;
};

struct VssIndexColumn {

    string name;
    sqlite3_int64 dimensions;
    string factory;
    faiss::MetricType metric;
    StorageType storage_type;
};

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

static void vssRangeSearchParamsFunc(sqlite3_context *context, int argc,
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

string get_index_filename(sqlite3 *db, const char *schema, const char *table_name, string col_name) {
    const char *db_filename = sqlite3_db_filename(db, "main");
    std::stringstream ss;
    ss << db_filename << "." << schema << "." << table_name << "." << col_name << ".faissindex";
    return ss.str();
}

void finalize_and_free(sqlite3_stmt *stmt, char *sql) {
    sqlite3_finalize(stmt);
    sqlite3_free(sql);
}

static int write_index_insert(faiss::Index *index,
                              sqlite3 *db,
                              char *schema,
                              char *name,
                              int rowId,
                              string col_name,
                              StorageType storage_type) {


    faiss::VectorIOWriter writer;
    faiss::write_index(index, &writer);
    sqlite3_int64 indexSize = writer.data.size();

    // First try to insert into xyz_index. If that fails with a rowid constraint
    // error, that means the index is already on disk, we just have to UPDATE
    // instead.

    if (storage_type == StorageType::faiss_ondisk) {
        const string index_filename = get_index_filename(db, schema, name, col_name);

        ofstream outFile(index_filename, ios::binary);
        if (!outFile) {
            return SQLITE_ERROR;
        }

        outFile.write(reinterpret_cast<const char*>(writer.data.data()), indexSize);
        outFile.close();
        if (!outFile) {
            return SQLITE_ERROR;
        }

        return SQLITE_OK;

    } else {

        sqlite3_stmt *stmt;
        char *sql = sqlite3_mprintf(
            "insert into \"%w\".\"%w_index\"(rowid, idx) values (?, ?)",
            schema,
            name);

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        if (rc != SQLITE_OK || stmt == nullptr) {
            sqlite3_free(sql);
            return SQLITE_ERROR;
        }

        rc = sqlite3_bind_int64(stmt, 1, rowId);
        if (rc != SQLITE_OK) {
            finalize_and_free(stmt, sql);
            return SQLITE_ERROR;
        }

        rc = sqlite3_bind_blob64(stmt, 2, writer.data.data(), indexSize, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            finalize_and_free(stmt, sql);
            return SQLITE_ERROR;
        }

        int result = sqlite3_step(stmt);
        finalize_and_free(stmt, sql);

        if (result == SQLITE_DONE) {

            // INSERT was success, index wasn't written yet, all good to exit
            return SQLITE_OK;

        } else if (sqlite3_extended_errcode(db) != SQLITE_CONSTRAINT_PRIMARYKEY) {
            // INSERT failed for another unknown reason, bad, return error
            return SQLITE_ERROR;
        }

        // INSERT failed because index already is on disk, so we do an UPDATE instead

        sql = sqlite3_mprintf(
            "update \"%w\".\"%w_index\" set idx = ? where rowid = ?", schema, name);

        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        if (rc != SQLITE_OK || stmt == nullptr) {
            sqlite3_free(sql);
            return SQLITE_ERROR;
        }

        rc = sqlite3_bind_blob64(stmt, 1, writer.data.data(), indexSize, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            finalize_and_free(stmt, sql);
            return SQLITE_ERROR;
        }

        rc = sqlite3_bind_int64(stmt, 2, rowId);
        if (rc != SQLITE_OK) {
            finalize_and_free(stmt, sql);
            return SQLITE_ERROR;
        }

        result = sqlite3_step(stmt);
        finalize_and_free(stmt, sql);

        if (result == SQLITE_DONE) {
            return SQLITE_OK;
        }

        return result;
    }
}

static int shadow_data_insert(sqlite3 *db,
                              char *schema,
                              char *name,
                              sqlite3_int64 *rowid,
                              sqlite3_int64 *retRowid) {

    sqlite3_stmt *stmt;

    if (rowid == nullptr) {

        auto sql = sqlite3_mprintf(
            "insert into \"%w\".\"%w_data\"(_) values (?)", schema, name);

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

        auto sql = sqlite3_mprintf(
            "insert into \"%w\".\"%w_data\"(rowid, _) values (?, ?);", schema,
            name);

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        sqlite3_free(sql);

        if (rc != SQLITE_OK || stmt == nullptr)
            return SQLITE_ERROR;

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

static int shadow_data_delete(sqlite3 *db,
                              char *schema,
                              char *name,
                              sqlite3_int64 rowid) {
    sqlite3_stmt *stmt;

    // TODO: We should strive to use only one concept and idea while creating
    // SQL statements.
    auto query = sqlite3_str_new(0);

    sqlite3_str_appendf(query, "delete from \"%w\".\"%w_data\" where rowid = ?",
                        schema, name);

    auto sql = sqlite3_str_finish(query);

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK || stmt == nullptr)
        return SQLITE_ERROR;

    sqlite3_bind_int64(stmt, 1, rowid);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return SQLITE_ERROR;
    }

    sqlite3_free(sql);
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

static faiss::Index *read_index_select(sqlite3 *db, const char *schema, const char *table_name, int indexId, string col_name, StorageType storage_type) {


    if (storage_type == StorageType::faiss_ondisk) {

        const string index_filename = get_index_filename(db, schema, table_name, col_name);
        return faiss::read_index(index_filename.c_str());

    } else {

        sqlite3_stmt *stmt;
        auto sql = sqlite3_mprintf("select idx from \"%w_index\" where rowid = ?", table_name);

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK || stmt == nullptr) {
            finalize_and_free(stmt, sql);
            return nullptr;
        }

        sqlite3_bind_int64(stmt, 1, indexId);
        if ((rc = sqlite3_step(stmt)) != SQLITE_ROW) {
            finalize_and_free(stmt, sql);
            return nullptr;
        }

        auto index_data = sqlite3_column_blob(stmt, 0);
        int64_t size = sqlite3_column_bytes(stmt, 0);

        faiss::VectorIOReader reader;
        copy((const uint8_t *)index_data,
            ((const uint8_t *)index_data) + size,
            back_inserter(reader.data));

        finalize_and_free(stmt, sql);

        return faiss::read_index(&reader);
    }
}

static int create_shadow_tables(sqlite3 *db,
                                const char *schema,
                                const char *name,
                                vector<vss_index *> indices) {


    // make the _index shadow tables if there's at least 1 column that uses the default faiss_shadow
    bool skip_shadow_index = true;
    for (auto i : indices) {
        if (i->storage_type == StorageType::faiss_shadow) {
            skip_shadow_index = false;
        }
    }

    if (!skip_shadow_index) {
        auto sql = sqlite3_mprintf("create table \"%w\".\"%w_index\"(rowid integer primary key autoincrement, idx)",
                                    schema,
                                    name);

        auto rc = sqlite3_exec(db, sql, 0, 0, 0);
        sqlite3_free(sql);
        if (rc != SQLITE_OK)
            return rc;

    }

    auto sql = sqlite3_mprintf("create table \"%w\".\"%w_data\"(rowid integer primary key autoincrement, _);",
                          schema,
                          name);

    auto rc = sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    sqlite3_free(sql);
    return rc;
}

static int drop_shadow_tables(sqlite3 *db, char *name) {

    const char *drops[2] = {"drop table \"%w_index\";",
                            "drop table \"%w_data\";"};

    for (int i = 0; i < 2; i++) {

        auto curSql = drops[i];

        sqlite3_stmt *stmt;

        // TODO: Use of one construct to create SQL statements.
        sqlite3_str *query = sqlite3_str_new(0);
        sqlite3_str_appendf(query, curSql, name);
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

// Tokens types when parsing vss0 column definitions
enum TokenType {
  LPAREN = 1,
  RPAREN = 2,
  EQUAL = 3,
  IDENTIFIER = 50,
  STRING = 51,
  INTEGER = 52
};

// Tokens types when parsing vss0 column definitions
struct Token {
public:
  TokenType token_type;
  // Only definied when token_type == TokenType::IDENTIFIER
  string identifier_value;
  // Only definied when token_type == TokenType::STRING
  string string_value;
  // Only definied when token_type == TokenType::INTEGER
  int int_value;

  explicit Token(TokenType token_type) : token_type(token_type) {}
  // TODO: maybe these should just be different classes that inherit Token? idk C++ man
  static Token IdentifierToken(string value){
    Token token(TokenType::IDENTIFIER);
    token.identifier_value = value;
    return token;
  }
  static Token StringToken(string value){
    Token token(TokenType::STRING);
    token.string_value = value;
    return token;
  }
  static Token IntToken(int value){
    Token token(TokenType::INTEGER);
    token.int_value = value;
    return token;
  }
};

// scans through a vss0 column definition string
struct Scanner {
public:
  explicit Scanner(string source): source(source), idx(0), len(source.length()) {}
  char advance() {
    return source[idx++];
  }
  optional<char> peek() {
    if(idx >= len) {
      return {};
    }
    return source[idx];
  }
  bool eof() {
    return idx >= len;
  }

private:
  string source;
  int idx;
  int len;

};

// Valid chatacters allowed in an identifier, except the 1st character (most be a-zA-Z_)
bool is_identifier_char(optional<char> c) {
  if(!c.has_value()) {
    return false;
  }
  return (c >= 'a' && c <= 'z')
  || (c >= 'A' && c <= 'Z')
  || (c >= '0' && c <= '9')
  || (c == '_');
}

// sus out the tokens in a vss0 column definition
vector<Token> tokenize(string source) {
  vector<Token> tokens;
  Scanner scanner(source);
  while (!scanner.eof()) {
    char c = scanner.advance();
    if(c == ' ' || c == '\n' || c == '\t') {
      continue;
    }
    else if(c=='(') {
      tokens.push_back(Token(TokenType::LPAREN));
    }
    else if(c==')') {
      tokens.push_back(Token(TokenType::RPAREN));
    }
    else if(c=='=') {
      tokens.push_back(Token(TokenType::EQUAL));
    }
    else if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      string identifier;
      identifier.push_back(c);
      while (is_identifier_char(scanner.peek())) {
        identifier.push_back(scanner.advance());
      }
      tokens.push_back(Token::IdentifierToken(identifier));

    }
    else if(c >= '0' && c <= '9') {
      string number_literal;
      number_literal.push_back(c);
      optional<char> next;
      while((next = scanner.peek()) && next.has_value() && next >= '0' && next <= '9') {
        number_literal.push_back(*next);
        scanner.advance();
      }
      tokens.push_back(Token::IntToken(atoi(number_literal.c_str())));
    }
    else if (c == '"') {
      string string_literal;
      optional<char> next;
      while(!scanner.eof() && (next = scanner.peek()) && next != '"') {
        string_literal.push_back(*next);
        scanner.advance();
      }
      if (next == '"') {
        scanner.advance();
        tokens.push_back(Token::StringToken(string_literal));
      }else {
        throw invalid_argument("unterminated string");
      }

    }
  }
  return tokens;
}

static const std::unordered_map<std::string, faiss::MetricType> metric_type_map = {
        {"L1", faiss::METRIC_L1},
        {"L2", faiss::METRIC_L2},
        {"INNER_PRODUCT", faiss::METRIC_INNER_PRODUCT},
        {"Linf", faiss::METRIC_Linf},
        // {"Lp", faiss::METRIC_Lp}, unavailable until metric arg is added
        {"Canberra", faiss::METRIC_Canberra},
        {"BrayCurtis", faiss::METRIC_BrayCurtis},
        {"JensenShannon", faiss::METRIC_JensenShannon}
    };

// parse a vss0 column definition. Throws on errors
VssIndexColumn parse_vss0_column_definition(string source) {
  string name;
  sqlite3_int64 dimensions;
  string factory = "Flat,IDMap2";
  faiss::MetricType metric_type = faiss::MetricType::METRIC_L2;
  StorageType storage_type = StorageType::faiss_shadow;

  vector<Token> tokens = tokenize(source);
  std::vector<Token>::iterator it = tokens.begin();

  // STEP 1: expect a column name as a first token (identifier)
  if(it == tokens.end()) {
    throw invalid_argument("No tokens available.");
  }
  if((*it).token_type != TokenType::IDENTIFIER) {
    throw invalid_argument("Expected identifier as first token, got ");
  }
  name = (*it).identifier_value;

  // STEP 2: ensure a '(' immediately follows
  it++;
  if(it == tokens.end()) {
    throw invalid_argument("Expected dimensions after column name declaration");
  }
  if((*it).token_type != TokenType::LPAREN) {
    throw invalid_argument("Expected left parethesis '('");
  }

  // STEP 3: ensure an integer token (# dimensions) immediately follows
  it++;
  if(it == tokens.end()) {
    throw invalid_argument("Expected dimensions after column name declaration");
  }
  if((*it).token_type != TokenType::INTEGER) {
    throw invalid_argument("Expected integer");
  }
  dimensions = (*it).int_value;

  // STEP 4: ensure a ')' token immediately follows
  it++;
  if(it == tokens.end()) {
    throw invalid_argument("Expected dimensions after column name declaration");
  }
  if((*it).token_type != TokenType::RPAREN) {
    throw invalid_argument("Expected right parethesis ')'");
  }

  // STEP 5: parse any column options
  it++;
  while(it != tokens.end()) {
    // every column option must start with an identifier
    if((*it).token_type != TokenType::IDENTIFIER) {
      throw invalid_argument("Expected an identifier for column arguments");
    }
    string key = (*it).identifier_value;
    if(key != "factory" && key != "metric_type" && key != "storage_type") {
      throw invalid_argument("Unknown vss0 column option '" + key + "'");
    }

    // ensure it is followed by an '='
    it++;
    if(it == tokens.end() || (*it).token_type != TokenType::EQUAL) {
      throw invalid_argument("Expected '=' ");
    }

    // now parse the value - different legal values for each key
    it++;
    if(it == tokens.end()) {
      throw invalid_argument("Expected column option value for " + key);
    }
    if (key == "factory") {
      if((*it).token_type != TokenType::STRING) {
        throw invalid_argument("Expected string value for factory column option, got ");
      }
      factory = (*it).string_value;
    }
    else if (key == "metric_type") {
      if((*it).token_type != TokenType::IDENTIFIER) {
        throw invalid_argument("Expected an identifier value for the 'metric_type' column option");
      }
      string value = (*it).identifier_value;
      auto it2 = metric_type_map.find(value);

      if( it2 == metric_type_map.end())  {
        throw invalid_argument("Unknown metric type: " + value);
      }

      metric_type = it2->second;
    }
    else if (key == "storage_type") {
      if((*it).token_type != TokenType::IDENTIFIER) {
        throw invalid_argument("Expected an identifier value for the 'storage_type' column option");
      }
      string value = (*it).identifier_value;
      if(value == "faiss_shadow") {
        storage_type = StorageType::faiss_shadow;
      }
      else if(value == "faiss_ondisk") {
        storage_type = StorageType::faiss_ondisk;
      }else {
        throw invalid_argument("storage_type value must be one of faiss_shadow or faiss_ondisk");
      }
    }

    it++;
  }
  return VssIndexColumn {
    name,
    dimensions,
    factory,
    metric_type,
    storage_type
  };
}

unique_ptr<vector<VssIndexColumn>> parse_constructor(int argc,
                                                     const char* const* argv,
                                                     sqlite3 *db) {
    auto columns = unique_ptr<vector<VssIndexColumn>>(new vector<VssIndexColumn>());

    for (int i = 3; i < argc; i++) {
        auto column = parse_vss0_column_definition(string(argv[i]));
        if (column.storage_type == StorageType::faiss_ondisk && sqlite3_db_filename(db, "main")[0] == '\0') {
            throw invalid_argument("Cannot use on disk storage for in memory db");
        }
        columns->push_back(column);
    }

    return columns;
}

static int init(sqlite3 *db,
                void *pAux,
                int argc,
                const char *const *argv,
                sqlite3_vtab **ppVtab,
                char **pzErr,
                bool isCreate) {

    sqlite3_vtab_config(db, SQLITE_VTAB_CONSTRAINT_SUPPORT, 1);
    int rc;

    sqlite3_str *str = sqlite3_str_new(nullptr);
    sqlite3_str_appendall(str,
                          "create table x(distance hidden, operation hidden");

    unique_ptr<vector<VssIndexColumn>> columns;
    try {
        columns = parse_constructor(argc, argv, db);
    } catch (const invalid_argument& e) {
        *pzErr = sqlite3_mprintf("Error parsing constructor: %s", e.what());
        return SQLITE_ERROR;
    }

    if (columns == nullptr) {
        *pzErr = sqlite3_mprintf("Error parsing constructor");
        return rc;
    }

    for (auto column = columns->begin(); column != columns->end(); ++column) {
        sqlite3_str_appendf(str, ", \"%w\"", column->name.c_str());
    }

    sqlite3_str_appendall(str, ")");
    auto sql = sqlite3_str_finish(str);
    rc = sqlite3_declare_vtab(db, sql);
    sqlite3_free(sql);

#define VSS_INDEX_COLUMN_DISTANCE 0
#define VSS_INDEX_COLUMN_OPERATION 1
#define VSS_INDEX_COLUMN_VECTORS 2

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

                auto index = faiss::index_factory(iter->dimensions, iter->factory.c_str(), iter->metric);
                pTable->indexes.push_back(new vss_index(index, iter->name, iter->storage_type));

            } catch (faiss::FaissException &e) {

                *pzErr = sqlite3_mprintf("Error building index factory for %s: %s",
                                         iter->name.c_str(),
                                         e.msg.c_str());

                return SQLITE_ERROR;
            }
        }

        rc = create_shadow_tables(db, argv[1], argv[2], pTable->indexes);
        if (rc != SQLITE_OK){
          *pzErr = sqlite3_mprintf("Error creating shadow tables");
          return rc;
        }


        // Shadow tables were successully created.
        // After shadow tables are created, write the initial index state to
        // shadow _index.
        auto i = 0;
        for (auto iter = pTable->indexes.begin(); iter != pTable->indexes.end(); ++iter, i++) {

            try {

                int rc = write_index_insert((*iter)->index,
                                            pTable->db,
                                            pTable->schema,
                                            pTable->name,
                                            i,
                                            (*iter)->name,
                                            (*iter)->storage_type);

                if (rc != SQLITE_OK) {
                  *pzErr = sqlite3_mprintf("Error initializing _index shadow tables");
                  return rc;
                }

            } catch (faiss::FaissException &e) {
              *pzErr = sqlite3_mprintf("Faiss error when initializing shadow tables: %s", e.what());
                return SQLITE_ERROR;
            }
        }

    } else {

        for (int i = 0; i < columns->size(); i++) {

            auto index = read_index_select(db, argv[1], argv[2], i, (*columns)[i].name, (*columns)[i].storage_type);

            // Index in shadow table should always be available, integrity check
            // to avoid null pointer
            if (index == nullptr) {
                *pzErr = sqlite3_mprintf("Could not read index at position %d", i);
                return SQLITE_ERROR;
            }
            pTable->indexes.push_back(new vss_index(index));
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
    drop_shadow_tables(pTable->db, pTable->name);
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

        pCursor->query_type = QueryType::search;
        vec_ptr query_vector;

        auto params = static_cast<VssSearchParams *>(sqlite3_value_pointer(argv[0], "vss0_searchparams"));
        if (params != nullptr) {

            pCursor->limit = params->k;
            query_vector = vec_ptr(new vector<float>(*params->vector));

        } else if (sqlite3_libversion_number() < 3041000) {

            // https://sqlite.org/forum/info/6b32f818ba1d97ef
            sqlite3_free(pVtabCursor->pVtab->zErrMsg);
            pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf(
                "vss_search() only support vss_search_params() as a "
                "2nd parameter for SQLite versions below 3.41.0");
            return SQLITE_ERROR;

        } else if ((query_vector = pCursor->table->vector_api->xValueAsVector(
                        argv[0])) != nullptr) {

            if (argc > 1) {
                pCursor->limit = sqlite3_value_int(argv[1]);
            } else {
                sqlite3_free(pVtabCursor->pVtab->zErrMsg);
                pVtabCursor->pVtab->zErrMsg =
                    sqlite3_mprintf("LIMIT required on vss_search() queries");
                return SQLITE_ERROR;
            }

        } else {

            if (pVtabCursor->pVtab->zErrMsg != nullptr)
                sqlite3_free(pVtabCursor->pVtab->zErrMsg);

            pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf(
                "2nd argument to vss_search() must be a vector");
            return SQLITE_ERROR;
        }

        int nq = 1;
        auto index = pCursor->table->indexes.at(idxNum)->index;

        if (query_vector->size() != index->d) {

            // TODO: To support index that transforms vectors
            // (to conserve spage, eg?), we should probably
            // have some logic in place that transforms the vectors here?
            sqlite3_free(pVtabCursor->pVtab->zErrMsg);
            pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf(
                "Input query size doesn't match index dimensions: %ld != %ld",
                query_vector->size(),
                index->d);
            return SQLITE_ERROR;
        }

        if (pCursor->limit <= 0) {

            sqlite3_free(pVtabCursor->pVtab->zErrMsg);
            pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf(
                "Limit must be greater than 0, got %ld", pCursor->limit);
            return SQLITE_ERROR;
        }

        // To avoid trying to select more records than number of records in index.
        auto searchMax = min(static_cast<faiss::idx_t>(pCursor->limit) * nq, index->ntotal * nq);

        pCursor->search_distances = vector<float>(searchMax, 0);
        pCursor->search_ids = vector<faiss::idx_t>(searchMax, 0);

        index->search(nq,
                      query_vector->data(),
                      searchMax,
                      pCursor->search_distances.data(),
                      pCursor->search_ids.data());

    } else if (strcmp(idxStr, "range_search") == 0) {

        pCursor->query_type = QueryType::range_search;

        auto params = static_cast<VssRangeSearchParams *>(
            sqlite3_value_pointer(argv[0], "vss0_rangesearchparams"));

        int nq = 1;

        vector<faiss::idx_t> nns(params->distance * nq);
        pCursor->range_search_result = unique_ptr<faiss::RangeSearchResult>(new faiss::RangeSearchResult(nq, true));

        auto index = pCursor->table->indexes.at(idxNum)->index;

        index->range_search(nq,
                            params->vector->data(),
                            params->distance,
                            pCursor->range_search_result.get());

    } else if (strcmp(idxStr, "fullscan") == 0) {

        pCursor->query_type = QueryType::fullscan;
        sqlite3_stmt *stmt;

        int res = sqlite3_prepare_v2(
            pCursor->table->db,
            sqlite3_mprintf("select rowid from \"%w_data\"", pCursor->table->name),
            -1, &pCursor->stmt, nullptr);

        if (res != SQLITE_OK)
            return res;

        pCursor->step_result = sqlite3_step(pCursor->stmt);

    } else {

        if (pVtabCursor->pVtab->zErrMsg != 0)
            sqlite3_free(pVtabCursor->pVtab->zErrMsg);

        pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf(
            "%s %s", "vssIndexFilter error: unhandled idxStr", idxStr);
        return SQLITE_ERROR;
    }

    pCursor->iCurrent = 0;
    return SQLITE_OK;
}

static int vssIndexNext(sqlite3_vtab_cursor *cur) {

    auto pCursor = static_cast<vss_index_cursor *>(cur);

    switch (pCursor->query_type) {

      case QueryType::search:
      case QueryType::range_search:
          pCursor->iCurrent++;
          break;

      case QueryType::fullscan:
          pCursor->step_result = sqlite3_step(pCursor->stmt);
    }

    return SQLITE_OK;
}

static int vssIndexRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid) {

    auto pCursor = static_cast<vss_index_cursor *>(cur);

    switch (pCursor->query_type) {

        case QueryType::search:
            *pRowid = pCursor->search_ids.at(pCursor->iCurrent);
            break;

        case QueryType::range_search:
            *pRowid = pCursor->range_search_result->labels[pCursor->iCurrent];
            break;

        case QueryType::fullscan:
            *pRowid = sqlite3_column_int64(pCursor->stmt, 0);
            break;
    }
    return SQLITE_OK;
}

static int vssIndexEof(sqlite3_vtab_cursor *cur) {

    auto pCursor = static_cast<vss_index_cursor *>(cur);

    switch (pCursor->query_type) {

      case QueryType::search:
          return pCursor->iCurrent >= pCursor->limit ||
                pCursor->iCurrent >= pCursor->search_ids.size()
                || (pCursor->search_ids.at(pCursor->iCurrent) == -1);

      case QueryType::range_search:
          return pCursor->iCurrent >= pCursor->range_search_result->lims[1];

      case QueryType::fullscan:
          return pCursor->step_result != SQLITE_ROW;
    }
    return 1;
}

static int vssIndexColumn(sqlite3_vtab_cursor *cur,
                          sqlite3_context *ctx,
                          int i) {

    auto pCursor = static_cast<vss_index_cursor *>(cur);

    if (i == VSS_INDEX_COLUMN_DISTANCE) {

        switch (pCursor->query_type) {

          case QueryType::search:
              sqlite3_result_double(ctx,
                                    pCursor->search_distances.at(pCursor->iCurrent));
              break;

          case QueryType::range_search:
              sqlite3_result_double(ctx,
                                    pCursor->range_search_result->distances[pCursor->iCurrent]);
              break;

          case QueryType::fullscan:
              break;
        }

    } else if (i >= VSS_INDEX_COLUMN_VECTORS) {

        auto index =
            pCursor->table->indexes.at(i - VSS_INDEX_COLUMN_VECTORS)->index;

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
        sqlite3_result_blob64(ctx, vec.data(), vec.size() * sizeof(float), SQLITE_TRANSIENT);
    }
    return SQLITE_OK;
}

static int vssIndexBegin(sqlite3_vtab *tab) {

    return SQLITE_OK;
}

static int vssIndexSync(sqlite3_vtab *pVTab) {

    auto pTable = static_cast<vss_index_vtab *>(pVTab);

    try {

        bool needsWriting = false;

        auto idxCol = 0;
        for (auto iter = pTable->indexes.begin(); iter != pTable->indexes.end(); ++iter, idxCol++) {

            // Checking if index needs training.
            if (!(*iter)->trainings.empty()) {

                (*iter)->index->train(
                    (*iter)->trainings.size() / (*iter)->index->d,
                    (*iter)->trainings.data());

                (*iter)->trainings.clear();
                (*iter)->trainings.shrink_to_fit();

                needsWriting = true;
            }

            // Checking if we're deleting records from the index.
            if (!(*iter)->delete_ids.empty()) {

                faiss::IDSelectorBatch selector((*iter)->delete_ids.size(),
                                                (*iter)->delete_ids.data());

                (*iter)->index->remove_ids(selector);
                (*iter)->delete_ids.clear();
                (*iter)->delete_ids.shrink_to_fit();

                needsWriting = true;
            }

            // Checking if we're inserting records to the index.
            if (!(*iter)->insert_data.empty()) {

                (*iter)->index->add_with_ids(
                    (*iter)->insert_ids.size(),
                    (*iter)->insert_data.data(),
                    (faiss::idx_t *)(*iter)->insert_ids.data());

                (*iter)->insert_ids.clear();
                (*iter)->insert_ids.shrink_to_fit();

                (*iter)->insert_data.clear();
                (*iter)->insert_data.shrink_to_fit();

                needsWriting = true;
            }
        }

        if (needsWriting) {

            int i = 0;
            for (auto iter = pTable->indexes.begin(); iter != pTable->indexes.end(); ++iter, i++) {

                int rc = write_index_insert((*iter)->index,
                                            pTable->db,
                                            pTable->schema,
                                            pTable->name,
                                            i,
                                            (*iter)->name,
                                            (*iter)->storage_type);

                if (rc != SQLITE_OK) {

                    sqlite3_free(pVTab->zErrMsg);
                    pVTab->zErrMsg = sqlite3_mprintf("Error saving index (%d): %s",
                                                    rc, sqlite3_errmsg(pTable->db));
                    return rc;
                }
            }
        }

        return SQLITE_OK;

    } catch (faiss::FaissException &e) {

        sqlite3_free(pVTab->zErrMsg);
        pVTab->zErrMsg =
            sqlite3_mprintf("Error during synchroning index. Full error: %s",
                            e.msg.c_str());

        for (auto iter = pTable->indexes.begin(); iter != pTable->indexes.end(); ++iter) {

            (*iter)->insert_ids.clear();
            (*iter)->insert_ids.shrink_to_fit();

            (*iter)->insert_data.clear();
            (*iter)->insert_data.shrink_to_fit();

            (*iter)->delete_ids.clear();
            (*iter)->delete_ids.shrink_to_fit();

            (*iter)->trainings.clear();
            (*iter)->trainings.shrink_to_fit();
        }

        return SQLITE_ERROR;
    }
}

static int vssIndexCommit(sqlite3_vtab *pVTab) { return SQLITE_OK; }

static int vssIndexRollback(sqlite3_vtab *pVTab) {

    auto pTable = static_cast<vss_index_vtab *>(pVTab);

    for (auto iter = pTable->indexes.begin(); iter != pTable->indexes.end(); ++iter) {

        (*iter)->trainings.clear();
        (*iter)->trainings.shrink_to_fit();

        (*iter)->insert_data.clear();
        (*iter)->insert_data.shrink_to_fit();

        (*iter)->insert_ids.clear();
        (*iter)->insert_ids.shrink_to_fit();

        (*iter)->delete_ids.clear();
        (*iter)->delete_ids.shrink_to_fit();
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

        auto rc = shadow_data_delete(pTable->db,
                                     pTable->schema,
                                     pTable->name,
                                     rowid_to_delete);
        if (rc != SQLITE_OK)
            return rc;

        for (auto iter = pTable->indexes.begin(); iter != pTable->indexes.end(); ++iter) {
            (*iter)->delete_ids.push_back(rowid_to_delete);
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
            for (auto iter = pTable->indexes.begin(); iter != pTable->indexes.end(); ++iter, i++) {

                if ((vec = pTable->vector_api->xValueAsVector(
                         argv[2 + VSS_INDEX_COLUMN_VECTORS + i])) != nullptr) {

                    // Make sure the index is already trained, if it's needed
                    if (!(*iter)->index->is_trained) {

                        sqlite3_free(pVTab->zErrMsg);
                        pVTab->zErrMsg =
                            sqlite3_mprintf("Index at i=%d requires training "
                                            "before inserting data.",
                                            i);

                        return SQLITE_ERROR;
                    }

                    if (!inserted_rowid) {

                        sqlite_int64 retrowid;
                        auto rc = shadow_data_insert(pTable->db, pTable->schema, pTable->name,
                                                     &rowid, &retrowid);
                        if (rc != SQLITE_OK)
                            return rc;

                        inserted_rowid = true;
                    }

                    (*iter)->insert_data.reserve((*iter)->insert_data.size() + vec->size());
                    (*iter)->insert_data.insert(
                        (*iter)->insert_data.end(),
                        vec->begin(),
                        vec->end());

                    (*iter)->insert_ids.push_back(rowid);

                    *pRowid = rowid;
                }
            }

        } else {

            string operation((char *)sqlite3_value_text(argv[2 + VSS_INDEX_COLUMN_OPERATION]));

            if (operation.compare("training") == 0) {

                auto i = 0;
                for (auto iter = pTable->indexes.begin(); iter != pTable->indexes.end(); ++iter, i++) {

                    vec_ptr vec = pTable->vector_api->xValueAsVector(argv[2 + VSS_INDEX_COLUMN_VECTORS + i]);
                    if (vec != nullptr) {

                        (*iter)->trainings.reserve((*iter)->trainings.size() + vec->size());
                        (*iter)->trainings.insert(
                            (*iter)->trainings.end(),
                            vec->begin(),
                            vec->end());
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
            sqlite3_mprintf("UPDATE statements on vss0 virtual tables not supported yet.");

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

    vector0_api *pRet = nullptr;
    sqlite3_stmt *pStmt = nullptr;

    auto rc = sqlite3_prepare(db, "select vector0(?1)", -1, &pStmt, nullptr);
    if (rc != SQLITE_OK)
        return nullptr;

    rc = sqlite3_bind_pointer(pStmt, 1, (void *)&pRet, "vector0_api_ptr", nullptr);
    if (rc != SQLITE_OK) {

        sqlite3_finalize(pStmt);
        return nullptr;
    }

    rc = sqlite3_step(pStmt);
    if (rc != SQLITE_ROW) {

        sqlite3_finalize(pStmt);
        return nullptr;
    }

    sqlite3_finalize(pStmt);
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
                                   0, 0, 0);

        sqlite3_create_function_v2(db,
                                   "vss_debug",
                                   0,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   0,
                                   vss_debug,
                                   0, 0, 0);

        sqlite3_create_function_v2(db,
                                   "vss_distance_l1",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_distance_l1,
                                   0, 0, 0);

        sqlite3_create_function_v2(db, "vss_distance_l2",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_distance_l2,
                                   0, 0, 0);

        sqlite3_create_function_v2(db, "vss_distance_linf",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_distance_linf,
                                   0, 0, 0);

        sqlite3_create_function_v2(db, "vss_inner_product",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_inner_product,
                                   0, 0, 0);

        sqlite3_create_function_v2(db, "vss_cosine_similarity",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_cosine_similarity,
                                   0, 0, 0);

        sqlite3_create_function_v2(db, "vss_fvec_add",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_fvec_add,
                                   0, 0, 0);

        sqlite3_create_function_v2(db, "vss_fvec_sub",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vss_fvec_sub,
                                   0, 0, 0);

        sqlite3_create_function_v2(db, "vss_search",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vssSearchFunc,
                                   0, 0, 0);

        sqlite3_create_function_v2(db,
                                   "vss_search_params",
                                   2,
                                   0,
                                   vector_api,
                                   vssSearchParamsFunc,
                                   0, 0, 0);

        sqlite3_create_function_v2(db,
                                   "vss_range_search",
                                   2,
                                   SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
                                   vector_api,
                                   vssRangeSearchFunc,
                                   0, 0, 0);

        sqlite3_create_function_v2(db,
                                   "vss_range_search_params",
                                   2,
                                   0,
                                   vector_api,
                                   vssRangeSearchParamsFunc,
                                   0, 0, 0);

        sqlite3_create_function_v2(db,
                                   "vss_memory_usage",
                                   0,
                                   0,
                                   nullptr,
                                   faissMemoryUsageFunc,
                                   0, 0, 0);

        auto rc = sqlite3_create_module_v2(db, "vss0", &vssIndexModule, vector_api, nullptr);
        if (rc != SQLITE_OK) {

            *pzErrMsg = sqlite3_mprintf("%s", sqlite3_errmsg(db));
            return rc;
        }

        return 0;
    }
}

#pragma endregion
