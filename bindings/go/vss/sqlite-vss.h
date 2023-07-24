
#ifndef _SQLITE_VSS_H
#define _SQLITE_VSS_H

#include "sqlite3ext.h"
#define SQLITE_VSS_VERSION "v0.1.2-alpha.2"

#define SQLITE_VSS_VERSION_MAJOR @sqlite - vss_VERSION_MAJOR @
#define SQLITE_VSS_VERSION_MINOR @sqlite - vss_VERSION_MINOR @
#define SQLITE_VSS_VERSION_PATCH @sqlite - vss_VERSION_PATCH @
#define SQLITE_VSS_VERSION_TWEAK @sqlite - vss_VERSION_TWEAK @

#ifdef __cplusplus
extern "C" {
#endif

int sqlite3_vss_init(sqlite3 *db,
                     char **pzErrMsg,
                     const sqlite3_api_routines *pApi);

#ifdef __cplusplus
} /* end of the 'extern "C"' block */
#endif

#endif /* ifndef _SQLITE_VSS_H */
