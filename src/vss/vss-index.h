
#ifndef VSS_INDEX_H
#define VSS_INDEX_H

#include "inclusions.h"

// Wrapper around a single faiss index, with training data, insert records, and
// delete records.
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

    vector<float> & getTrainings() {

        return trainings;
    }

    vector<float> & getInsert_data() {

        return insert_data;
    }

    vector<faiss::idx_t> & getInsert_ids() {

        return insert_ids;
    }

    vector<faiss::idx_t> & getDelete_ids() {

        return delete_ids;
    }

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

    faiss::Index *index;
    vector<float> trainings;
    vector<float> insert_data;
    vector<faiss::idx_t> insert_ids;
    vector<faiss::idx_t> delete_ids;
};

#endif // VSS_INDEX_H
