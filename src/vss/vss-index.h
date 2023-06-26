
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

private:

    faiss::Index *index;
    vector<float> trainings;
    vector<float> insert_data;
    vector<faiss::idx_t> insert_ids;
    vector<faiss::idx_t> delete_ids;
};

#endif // VSS_INDEX_H
