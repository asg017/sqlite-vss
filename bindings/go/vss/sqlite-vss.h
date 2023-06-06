
#ifndef _SQLITE_VSS_H
#define _SQLITE_VSS_H

#include "sqlite3ext.h"
#define SQLITE_VSS_VERSION "v0.1.1-alpha.18"

#define SQLITE_VSS_VERSION_MAJOR 0
#define SQLITE_VSS_VERSION_MINOR 1
#define SQLITE_VSS_VERSION_PATCH 1
#define SQLITE_VSS_VERSION_TWEAK 18


#ifdef __cplusplus
extern "C" {
#endif

int sqlite3_vss_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

#ifdef __cplusplus
}  /* end of the 'extern "C"' block */
#endif

#endif /* ifndef _SQLITE_VSS_H */
