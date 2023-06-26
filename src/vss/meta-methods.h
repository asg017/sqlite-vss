
#ifndef META_METHODS_H
#define META_METHODS_H

#include "sqlite-vss.h"
#include <faiss/index_io.h>
#include <faiss/utils/utils.h>


static void vss_version(sqlite3_context *context,
                        int argc,
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


#endif // META_METHODS_H

