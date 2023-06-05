#include "sqlite3.h"
#include "sqlite-vector.h"
#include "sqlite-vss.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int rc = SQLITE_OK;
  sqlite3 *db;
  sqlite3_stmt *stmt;
  char* error_message;

  // Call the sqlite-vector and sqlite-vss entrypoints on every new SQLite database.
  rc = sqlite3_auto_extension((void (*)())sqlite3_vector_init);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "❌ demo.c could not load sqlite3_vector_init: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }
  rc = sqlite3_auto_extension((void (*)())sqlite3_vss_init);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "❌ demo.c could not load sqlite3_vss_init: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }

  // this database connection will now have all sqlite-vss SQL functions available
  rc = sqlite3_open(":memory:", &db);

  if (rc != SQLITE_OK) {
    fprintf(stderr, "❌ demo.c cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }

  rc = sqlite3_prepare_v2(db, "SELECT vss_version(), vector_to_json(?)", -1, &stmt, NULL);
  if(rc != SQLITE_OK) {
    fprintf(stderr, "❌ demo.c%s\n", error_message);
    sqlite3_free(error_message);
    sqlite3_close(db);
    return 1;
  }
  unsigned char blob[] = {0x00, 0x0, 0x28, 0x42};
  sqlite3_bind_blob(stmt, 1, blob, sizeof(blob), SQLITE_STATIC);
  if (SQLITE_ROW != sqlite3_step(stmt)) {
    fprintf(stderr, "❌ demo.c%s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }
  printf("version=%s vector=%s\n", sqlite3_column_text(stmt, 0), sqlite3_column_text(stmt, 1));
  sqlite3_finalize(stmt);

  rc = sqlite3_exec(db,
    "CREATE VIRTUAL TABLE vss_demo USING vss0(a(2)); \
    INSERT INTO vss_demo(rowid, a) \
      VALUES \
        (1, '[1.0, 2.0]'), \
        (2, '[2.0, 2.0]'), \
        (3, '[3.0, 2.0]')  \
    "
  , NULL, NULL, &error_message);

  if (rc != SQLITE_OK) {
      fprintf(stderr, "❌%s\n", error_message);
      sqlite3_free(error_message);
      sqlite3_close(db);
      return 1;
  }

  rc = sqlite3_prepare_v2(db, "\
    SELECT \
      rowid, \
      distance \
    FROM vss_demo \
    WHERE vss_search(a, '[1.0, 2.0]') \
    LIMIT 10;\
    ", -1, &stmt, NULL);
  if(rc != SQLITE_OK) {
    fprintf(stderr, "❌ demo.c %s\n", sqlite3_errmsg(db));
      sqlite3_free(error_message);
      sqlite3_close(db);
      return 1;
  }
  while (SQLITE_ROW == (rc = sqlite3_step(stmt))) {
    long rowid = sqlite3_column_int64(stmt, 0);
    double distance = sqlite3_column_double(stmt, 1);
    printf("rowid=%ld distance=%f\n", rowid, distance);
  }
  if(rc != SQLITE_DONE) {
    fprintf(stderr, "❌ demo.c %s\n", sqlite3_errmsg(db));
      sqlite3_free(error_message);
      sqlite3_close(db);
      sqlite3_finalize(stmt);
      return 1;
  }
  sqlite3_finalize(stmt);

  printf("✅ demo.c ran successfully. \n");
  sqlite3_close(db);
  return 0;
}

