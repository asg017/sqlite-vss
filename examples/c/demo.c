#include "sqlite3.h"
#include "sqlite-vector.h"
#include "sqlite-vss.h"
#include <stdio.h>
#include <unistd.h>

// callback to sqlite3_exec that prints the column names + row values of a query response
int print_results(void* pData, int argc, char** argv, char** columns) {
  for (int i = 0; i < argc; i++) {
      printf("%s = %s\n", columns[i], argv[i] ? argv[i] : "NULL");
  }
  return SQLITE_OK;
}

int main(int argc, char *argv[]) {
  int rc = SQLITE_OK;
  sqlite3 *db;
  char* error_message;

  // Call the sqlite-vector and sqlite-vss entrypoints on every new SQLite database.

  rc = sqlite3_auto_extension((void (*)())sqlite3_vector_init);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "❌ demo.c could not load sqlite3_hello_init: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }
  rc = sqlite3_auto_extension((void (*)())sqlite3_vss_init);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "❌ demo.c could not load sqlite3_hola_init: %s\n", sqlite3_errmsg(db));
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

  rc = sqlite3_exec(db, "SELECT vss_version(), vector_to_json(X'0000284200000000')", print_results, NULL, &error_message);
  if (rc != SQLITE_OK) {
      fprintf(stderr, "❌ demo.c error executing query: %s\n", error_message);
      sqlite3_free(error_message);
      sqlite3_close(db);
      return 1;
  }

  printf("✅ demo.c ran successfully. \n");
  sqlite3_close(db);
  return 0;
}

