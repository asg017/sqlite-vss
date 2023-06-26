
#ifndef VSS_CALCULATIONS_H
#define VSS_CALCULATIONS_H

#include "sqlite-vss.h"
#include <faiss/index_io.h>
#include <faiss/utils/utils.h>

using namespace std;

typedef unique_ptr<vector<float>> vec_ptr;

static void vss_distance_l1(sqlite3_context *context,
                            int argc,
                            sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size",
                             -1);
        return;
    }

    sqlite3_result_double(context, faiss::fvec_L1(lhs->data(), rhs->data(), lhs->size()));
}

static void vss_distance_l2(sqlite3_context *context, int argc,
                            sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size",
                             -1);
        return;
    }

    sqlite3_result_double(context, faiss::fvec_L2sqr(lhs->data(), rhs->data(), lhs->size()));
}

static void vss_distance_linf(sqlite3_context *context, int argc,
                              sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size",
                             -1);
        return;
    }

    sqlite3_result_double(context, faiss::fvec_Linf(lhs->data(), rhs->data(), lhs->size()));
}

static void vss_inner_product(sqlite3_context *context, int argc,
                              sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size",
                             -1);
        return;
    }

    sqlite3_result_double(context,
                          faiss::fvec_inner_product(lhs->data(), rhs->data(), lhs->size()));
}

static void vss_fvec_add(sqlite3_context *context, int argc,
                         sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size",
                             -1);
        return;
    }

    auto size = lhs->size();
    vec_ptr c(new vector<float>(size));
    faiss::fvec_add(size, lhs->data(), rhs->data(), c->data());

    vector_api->xResultVector(context, c.get());
}

static void vss_fvec_sub(sqlite3_context *context, int argc,
                         sqlite3_value **argv) {

    auto vector_api = (vector0_api *)sqlite3_user_data(context);

    vec_ptr lhs = vector_api->xValueAsVector(argv[0]);
    if (lhs == nullptr) {
        sqlite3_result_error(context, "LHS is not a vector", -1);
        return;
    }

    vec_ptr rhs = vector_api->xValueAsVector(argv[1]);
    if (rhs == nullptr) {
        sqlite3_result_error(context, "RHS is not a vector", -1);
        return;
    }

    if (lhs->size() != rhs->size()) {
        sqlite3_result_error(context, "LHS and RHS are not vectors of the same size", -1);
        return;
    }

    int size = lhs->size();
    vec_ptr c = vec_ptr(new vector<float>(size));
    faiss::fvec_sub(size, lhs->data(), rhs->data(), c->data());
    vector_api->xResultVector(context, c.get());
}

#endif // VSS_CALCULATIONS_H
