
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

    explicit vss_index(faiss::Index *index) : index(index) {}

    ~vss_index() {

        if (index != nullptr) {
            delete index;
        }
    }

    faiss::Index * getIndex() {

        return index;
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
        if (tryDelete())
            result = true;
        if (tryInsert())
            result = true;

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

private:

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

    faiss::Index *index;
    vector<float> trainings;
    vector<float> insert_data;
    vector<faiss::idx_t> insert_ids;
    vector<faiss::idx_t> delete_ids;
};

#endif // VSS_INDEX_H
