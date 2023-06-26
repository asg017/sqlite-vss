
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

    void addTrainings(vec_ptr & vec) {

        trainings.reserve(trainings.size() + vec->size());
        trainings.insert(trainings.end(), vec->begin(), vec->end());
    }

    void addInsertData(faiss::idx_t rowId, vec_ptr & vec) {

        insert_data.reserve(insert_data.size() + vec->size());
        insert_data.insert(insert_data.end(), vec->begin(), vec->end());

        insert_ids.push_back(rowId);
    }

    void addDelete(faiss::idx_t rowid) {

        delete_ids.push_back(rowid);
    }

    bool synchronize() {

        auto result = tryTrain();
        result = tryDelete() || result;
        result = tryInsert() || result;

        // Now that we've updated our faiss::index we delete all temporary data.
        reset();

        return result;
    }

    void reset() {

        trainings.clear();
        trainings.shrink_to_fit();

        insert_ids.clear();
        insert_ids.shrink_to_fit();

        insert_data.clear();
        insert_data.shrink_to_fit();

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

    bool tryInsert() {

        if (insert_ids.empty())
            return false;

        index->add_with_ids(
            insert_ids.size(),
            insert_data.data(),
            (faiss::idx_t *)insert_ids.data());

        insert_ids.clear();
        insert_ids.shrink_to_fit();

        insert_data.clear();
        insert_data.shrink_to_fit();

        return true;
    }

    faiss::Index *index;
    vector<float> trainings;
    vector<float> insert_data;
    vector<faiss::idx_t> insert_ids;
    vector<faiss::idx_t> delete_ids;
};

#endif // VSS_INDEX_H
