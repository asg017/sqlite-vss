
#ifndef VSS_INDEX_H
#define VSS_INDEX_H

#include "inclusions.h"
/*
 * Wrapper around a single faiss index, with training data, insert records, and
 * delete records.
 * 
 * An attempt at encapsulating everything related to faiss::Index instances, such as
 * training, inserting, deleting, etc.
 */
class vss_index {

public:

    ~vss_index() {

        if (index != nullptr) {
            delete index;
        }
    }

    // Returns false if index requires training before inserting items to it.
    bool isTrained() {

        return index->is_trained;
    }

    // Reconstructs the original vector, requires IDMap2 string in index factory to work.
    void reconstruct(sqlite3_int64 rowid, vector<float> & vector) {

        index->reconstruct(rowid, vector.data());
    }

    // Returns true if specified vector is allowed to query index.
    bool canQuery(vec_ptr & vec) {

        return vec->size() == index->d;
    }

    // Queries the index for matches matching the specified vector
    void search(int nq,
                vec_ptr & vec,
                faiss::idx_t max,
                vector<float> & distances,
                vector<faiss::idx_t> & ids) {

        index->search(nq, vec->data(), max, distances.data(), ids.data());
    }

    // Queries the index for a range of items.
    void range_search(int nq, vec_ptr & vec, float distance, unique_ptr<faiss::RangeSearchResult> & result) {

        index->range_search(nq, vec->data(), distance, result.get());
    }

    // Returns dimensions of index.
    faiss::idx_t dimensions() {

        return index->d;
    }

    // Returns the size of index.
    faiss::idx_t size() {

        return index->ntotal;
    }

    /*
     * Adds the specified vector to the index' training material.
     *
     * Notice, needs to invoke synchronize() later to actually perform training of index.
     */
    void addTrainings(vec_ptr & vec) {

        trainings.reserve(trainings.size() + vec->size());
        trainings.insert(trainings.end(), vec->begin(), vec->end());
    }

    /*
     * Adds the specified vector to the index' temporary insert data.
     *
     * Notice, needs to invoke synchronize() later to actually add data to index.
     */
    void addInsertData(faiss::idx_t rowId, vec_ptr & vec) {

        insert_data.reserve(insert_data.size() + vec->size());
        insert_data.insert(insert_data.end(), vec->begin(), vec->end());

        insert_ids.push_back(rowId);
    }

    /*
     * Adds the specified rowid to the index' temporary delete data.
     *
     * Notice, needs to invoke synchronize() later to actually delete data from index.
     */
    void addDelete(faiss::idx_t rowid) {

        delete_ids.push_back(rowid);
    }

    /*
     * Synchronizes index by updating index according to trainings, inserts and deletes.
     */
    bool synchronize() {

        auto result = tryTrain();
        result = tryDelete() || result;
        result = tryInsert() || result;

        return result;
    }

    /*
     * Resets all temporary training data to free memory.
     */
    void reset() {

        trainings.clear();
        trainings.shrink_to_fit();

        insert_data.clear();
        insert_data.shrink_to_fit();

        insert_ids.clear();
        insert_ids.shrink_to_fit();

        delete_ids.clear();
        delete_ids.shrink_to_fit();
    }

    int write_index(sqlite3 *db,
                    const char *schema,
                    const char *name,
                    int rowId) {

        // Writing our index
        faiss::VectorIOWriter writer;
        faiss::write_index(index, &writer);

        // First trying to insert index, if that fails with ROW constraing error, we try to update existing index.
        if (write_index_insert(writer, db, schema, name, rowId) == SQLITE_OK)
            return SQLITE_OK;

        if (sqlite3_extended_errcode(db) != SQLITE_CONSTRAINT_ROWID)
            return SQLITE_ERROR; // Insert failed for unknown error

        // Insert failed because index already existed, updating existing index.
        return write_index_update(writer, db, schema, name, rowId);
    }

    /*
     * Creates a new vss_index as a virtual table and stores
     * its initial (empty) state.
     */
    static vss_index * factory(sqlite3 *db,
                               const char *schema,
                               const char *name,
                               bool  indexNo,
                               string & factoryArgs,
                               int dimensions) {

        // Creating a new index and storing in cache.
        auto newIndex = new vss_index(faiss::index_factory(dimensions, factoryArgs.c_str()));

        // Checking if this is our first index for table, at which point we create our shadow tables.
        if (indexNo == 0) {

            auto rc = create_shadow_tables(db, schema, name);
            if (rc != SQLITE_OK)
                throw domain_error("Couldn't create shadow tables");
        }

        // Writing its initial (empty) state.
        int rc = newIndex->write_index(db,
                                       schema,
                                       name,
                                       indexNo);

        // Returning index to caller.
        return newIndex;
    }

    /*
     * Creates a new vss_index by reading existing data from db,
     * or returns a cached index to caller.
     */
    static vss_index * factory(sqlite3 *db,
                               const char *name,
                               int indexNo) {

        // Reading index from db.
        auto newIndex = new vss_index(read_index_select(db, name, indexNo));

        // Returning index to caller.
        return newIndex;
    }

private:

    explicit vss_index(faiss::Index *index) : index(index) { }

    static int create_shadow_tables(sqlite3 *db,
                                    const char *schema,
                                    const char *name) {

        SqlStatement create1(db,
                             sqlite3_mprintf("create table \"%w\".\"%w_index\"(idx)",
                                             schema,
                                             name));

        auto rc = create1.exec();
        if (rc != SQLITE_OK)
            return rc;

        /*
         * Notice, we'll need to explicitly finalize this object since we can only
         * have one open statement at the same time to the same connetion.
         */
        create1.finalize();

        SqlStatement create2(db,
                             sqlite3_mprintf("create table \"%w\".\"%w_data\"(x);",
                                             schema,
                                             name));

        rc = create2.exec();
        return rc;
    }

    static faiss::Index * read_index_select(sqlite3 *db,
                                            const char *name,
                                            int indexId) {

        SqlStatement select(db,
                            sqlite3_mprintf("select idx from \"%w_index\" where rowid = ?",
                                            name));

        if (select.prepare() != SQLITE_OK)
            return nullptr;

        if (select.bind_int64(1, indexId) != SQLITE_OK)
            return nullptr;

        if (select.step() != SQLITE_ROW)
            return nullptr;

        auto index_data = select.column_blob(0);
        auto size = select.column_bytes(0);

        faiss::VectorIOReader reader;
        copy((const uint8_t *)index_data,
            ((const uint8_t *)index_data) + size,
            back_inserter(reader.data));

        return faiss::read_index(&reader);
    }

    int write_index_insert(faiss::VectorIOWriter &writer,
                           sqlite3 *db,
                           const char *schema,
                           const char *name,
                           int rowId) {

        // If inserts fails it means index already exists.
        SqlStatement insert(db,
                            sqlite3_mprintf("insert into \"%w\".\"%w_index\"(rowid, idx) values (?, ?)",
                                            schema,
                                            name));

        if (insert.prepare() != SQLITE_OK)
            return SQLITE_ERROR;

        if (insert.bind_int64(1, rowId) != SQLITE_OK)
            return SQLITE_ERROR;

        if (insert.bind_blob64(2, writer.data.data(), writer.data.size()) != SQLITE_OK)
            return SQLITE_ERROR;

        auto rc = insert.step();
        if (rc == SQLITE_DONE)
            return SQLITE_OK; // Index did not exist, and we successfully inserted it.

        return rc;
    }

    int write_index_update(faiss::VectorIOWriter &writer,
                           sqlite3 *db,
                           const char *schema,
                           const char *name,
                           int rowId) {

        // Updating existing index.
        SqlStatement update(db,
                            sqlite3_mprintf("update \"%w\".\"%w_index\" set idx = ? where rowid = ?",
                                            schema,
                                            name));

        if (update.prepare() != SQLITE_OK)
            return SQLITE_ERROR;

        if (update.bind_blob64(1, writer.data.data(), writer.data.size()) != SQLITE_OK)
            return SQLITE_ERROR;

        if (update.bind_int64(2, rowId) != SQLITE_OK)
            return SQLITE_ERROR;

        auto rc = update.step();
        if (rc == SQLITE_DONE)
            return SQLITE_OK; // We successfully updated existing index.

        return rc;
    }

    bool tryTrain() {

        if (trainings.empty())
            return false;

        index->train(trainings.size() / index->d, trainings.data());

        trainings.clear();
        trainings.shrink_to_fit();

        return true;
    }

    bool tryInsert() {

        if (insert_ids.empty())
            return false;

        index->add_with_ids(
            insert_ids.size(),
            insert_data.data(),
            insert_ids.data());

        insert_ids.clear();
        insert_ids.shrink_to_fit();

        insert_data.clear();
        insert_data.shrink_to_fit();

        return true;
    }

    bool tryDelete() {

        if (delete_ids.empty())
            return false;

        faiss::IDSelectorBatch selector(delete_ids.size(),
                                        delete_ids.data());

        index->remove_ids(selector);

        delete_ids.clear();
        delete_ids.shrink_to_fit();

        return true;
    }

    faiss::Index * index;
    vector<float> trainings;
    vector<float> insert_data;
    vector<faiss::idx_t> insert_ids;
    vector<faiss::idx_t> delete_ids;
};

#endif // VSS_INDEX_H
