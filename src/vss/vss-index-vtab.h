
#ifndef VSS_INDEX_VTAB_H
#define VSS_INDEX_VTAB_H

#include "inclusions.h"

class vss_index_vtab : public sqlite3_vtab {

public:

    vss_index_vtab(sqlite3 *db, vector0_api *vector_api, char *schema, char *name)
      : db(db),
        vector_api(vector_api),
        schema(schema),
        name(name) {

        this->zErrMsg = nullptr;
    }

    ~vss_index_vtab() {

        if (name)
            sqlite3_free(name);

        if (schema)
            sqlite3_free(schema);

        if (this->zErrMsg != nullptr)
            delete this->zErrMsg;

        // Deleting all indexes associated with table.
        for (auto iter = indexes.begin(); iter != indexes.end(); ++iter) {
            delete (*iter);
        }
    }

    void setError(char *error) {

        if (this->zErrMsg != nullptr) {
            delete this->zErrMsg;
        }

        this->zErrMsg = error;
    }

    sqlite3 * getDb() {

        return db;
    }

    vector0_api * getVector0_api() {

        return vector_api;
    }

    vector<vss_index *> & getIndexes() {

        return indexes;
    }

    char * getName() {

        return name;
    }

    char * getSchema() {

        return schema;
    }

private:

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

#endif // VSS_INDEX_VTAB_H
