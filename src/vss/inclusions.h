
#ifndef VSS_INCLUSIONS_H
#define VSS_INCLUSIONS_H

#include <cstdio>
#include <cstdlib>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>

#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFPQ.h>
#include <faiss/impl/AuxIndexStructures.h>
#include <faiss/impl/IDSelector.h>
#include <faiss/impl/io.h>
#include <faiss/index_factory.h>
#include <faiss/index_io.h>
#include <faiss/utils/distances.h>
#include <faiss/utils/utils.h>

using namespace std;

typedef unique_ptr<vector<float>> vec_ptr;

enum QueryType { search, range_search, fullscan };

#define VSS_SEARCH_FUNCTION SQLITE_INDEX_CONSTRAINT_FUNCTION
#define VSS_RANGE_SEARCH_FUNCTION SQLITE_INDEX_CONSTRAINT_FUNCTION + 1

#endif // VSS_INCLUSIONS_H
