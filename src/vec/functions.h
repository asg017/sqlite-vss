
#ifndef TRANSFORMERS_H
#define TRANSFORMERS_H

char VECTOR_BLOB_HEADER_BYTE = 'v';
char VECTOR_BLOB_HEADER_TYPE = 1;
const char *VECTOR_FLOAT_POINTER_NAME = "vectorf32v0";

// https://github.com/sqlite/sqlite/blob/master/src/json.c#L88-L89
#define JSON_SUBTYPE 74 /* Ascii for "J" */

#include <nlohmann/json.hpp>
using json = nlohmann::json;

void delVectorFloat(void *p) {

    auto vx = static_cast<VectorFloat *>(p);
    sqlite3_free(vx->data);
    delete vx;
}

void resultVector(sqlite3_context *context, vector<float> *vecIn) {

    auto vecRes = new VectorFloat();

    vecRes->size = vecIn->size();
    vecRes->data = (float *)sqlite3_malloc(vecIn->size() * sizeof(float));

    memcpy(vecRes->data, vecIn->data(), vecIn->size() * sizeof(float));

    sqlite3_result_pointer(context, vecRes, VECTOR_FLOAT_POINTER_NAME, delVectorFloat);
}

vec_ptr vectorFromBlobValue(sqlite3_value *value, const char **pzErrMsg) {

    int size = sqlite3_value_bytes(value);
    char header;
    char type;

    if (size < (2)) {
        *pzErrMsg = "Vector blob size less than header length";
        return nullptr;
    }

    const void *pBlob = sqlite3_value_blob(value);
    memcpy(&header, ((char *)pBlob + 0), sizeof(char));
    memcpy(&type, ((char *)pBlob + 1), sizeof(char));

    if (header != VECTOR_BLOB_HEADER_BYTE) {
        *pzErrMsg = "Blob not well-formatted vector blob";
        return nullptr;
    }

    if (type != VECTOR_BLOB_HEADER_TYPE) {
        *pzErrMsg = "Blob type not right";
        return nullptr;
    }

    int numElements = (size - 2) / sizeof(float);
    float *vec = (float *)((char *)pBlob + 2);
    return vec_ptr(new vector<float>(vec, vec + numElements));
}

vec_ptr vectorFromRawBlobValue(sqlite3_value *value, const char **pzErrMsg) {

    int size = sqlite3_value_bytes(value);

    // Must be divisible by 4
    if (size % 4) {
        *pzErrMsg = "Invalid raw blob length, blob must be divisible by 4";
        return nullptr;
    }
    const void *pBlob = sqlite3_value_blob(value);

    float *vec = (float *)((char *)pBlob);
    return vec_ptr(new vector<float>(vec, vec + (size / 4)));
}

vec_ptr vectorFromTextValue(sqlite3_value *value) {

    try {

        json json = json::parse(sqlite3_value_text(value));
        vec_ptr pVec(new vector<float>());
        json.get_to(*pVec);
        return pVec;

    } catch (const json::exception &) {
        return nullptr;
    }

    return nullptr;
}

static vec_ptr valueAsVector(sqlite3_value *value) {

    // Option 1: If the value is a "vectorf32v0" pointer, create vector from
    // that
    auto vec = (VectorFloat *)sqlite3_value_pointer(value, VECTOR_FLOAT_POINTER_NAME);

    if (vec != nullptr)
        return vec_ptr(new vector<float>(vec->data, vec->data + vec->size));

    vec_ptr pVec;

    // Option 2: value is a blob in vector format
    if (sqlite3_value_type(value) == SQLITE_BLOB) {

        const char *pzErrMsg = nullptr;

        if ((pVec = vectorFromBlobValue(value, &pzErrMsg)) != nullptr)
            return pVec;

        if ((pVec = vectorFromRawBlobValue(value, &pzErrMsg)) != nullptr)
            return pVec;
    }

    // Option 3: if value is a JSON array coercible to float vector, use that
    if (sqlite3_value_type(value) == SQLITE_TEXT) {

        if ((pVec = vectorFromTextValue(value)) != nullptr)
            return pVec;
        else
            return nullptr;
    }

    // Else, value isn't a vector
    return nullptr;
}

// TODO should return fvec, ivec, or bvec depending on input. How do bvec,
// though?
static void vector_from(sqlite3_context *context,
                        int argc,
                        sqlite3_value **argv) {

    vector<float> vec;
    vec.reserve(argc);
    for (int i = 0; i < argc; i++) {
        vec.push_back(sqlite3_value_double(argv[i]));
    }

    resultVector(context, &vec);
}

static void vector_value_at(sqlite3_context *context,
                            int argc,
                            sqlite3_value **argv) {

    vec_ptr pVec = valueAsVector(argv[0]);

    if (pVec == nullptr)
        return;

    int pos = sqlite3_value_int(argv[1]);

    try {

        float result = pVec->at(pos);
        sqlite3_result_double(context, result);

    } catch (const out_of_range &oor) {

        char *errmsg = sqlite3_mprintf("%d out of range: %s", pos, oor.what());

        if (errmsg != nullptr) {
            sqlite3_result_error(context, errmsg, -1);
            sqlite3_free(errmsg);
        } else {
            sqlite3_result_error_nomem(context);
        }
    }
}

static void vector_length(sqlite3_context *context,
                          int argc,
                          sqlite3_value **argv) {

    auto pVec = (VectorFloat *)sqlite3_value_pointer(argv[0], VECTOR_FLOAT_POINTER_NAME);
    if (pVec == nullptr)
        return;

    sqlite3_result_int64(context, pVec->size);
}

static void vector_to_json(sqlite3_context *context,
                           int argc,
                           sqlite3_value **argv) {

    vec_ptr pVec = valueAsVector(argv[0]);
    if (pVec == nullptr)
        return;

    json j = json(*pVec);

    sqlite3_result_text(context, j.dump().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_result_subtype(context, JSON_SUBTYPE);
}

static void vector_from_json(sqlite3_context *context,
                             int argc,
                             sqlite3_value **argv) {

    const char *text = (const char *)sqlite3_value_text(argv[0]);
    vec_ptr pVec = vectorFromTextValue(argv[0]);

    if (pVec == nullptr) {
        sqlite3_result_error(
            context, "input not valid json, or contains non-float data", -1);
    } else {
        resultVector(context, pVec.get());
    }
}

static void vector_to_blob(sqlite3_context *context,
                           int argc,
                           sqlite3_value **argv) {

    vec_ptr pVec = valueAsVector(argv[0]);
    if (pVec == nullptr)
        return;

    int size = pVec->size();
    int memSize = (sizeof(char)) + (sizeof(char)) + (size * 4);
    void *pBlob = sqlite3_malloc(memSize);
    memset(pBlob, 0, memSize);

    memcpy((void *)((char *)pBlob + 0), (void *)&VECTOR_BLOB_HEADER_BYTE, sizeof(char));
    memcpy((void *)((char *)pBlob + 1), (void *)&VECTOR_BLOB_HEADER_TYPE, sizeof(char));
    memcpy((void *)((char *)pBlob + 2), (void *)pVec->data(), size * 4);

    sqlite3_result_blob64(context, pBlob, memSize, sqlite3_free);
}

static void vector_from_blob(sqlite3_context *context,
                             int argc,
                             sqlite3_value **argv) {

    const char *pzErrMsg;

    vec_ptr pVec = vectorFromBlobValue(argv[0], &pzErrMsg);
    if (pVec == nullptr)
        sqlite3_result_error(context, pzErrMsg, -1);
    else
        resultVector(context, pVec.get());
}

static void vector_to_raw(sqlite3_context *context,
                          int argc,
                          sqlite3_value **argv) {

    vec_ptr pVec = valueAsVector(argv[0]);
    if (pVec == nullptr)
        return;

    int size = pVec->size();
    int n = size * sizeof(float);
    void *pBlob = sqlite3_malloc(n);
    memset(pBlob, 0, n);
    memcpy((void *)((char *)pBlob), (void *)pVec->data(), n);
    sqlite3_result_blob64(context, pBlob, n, sqlite3_free);
}

static void vector_from_raw(sqlite3_context *context,
                            int argc,
                            sqlite3_value **argv) {

    const char *pzErrMsg; // TODO: Shouldn't we have like error messages here?

    vec_ptr pVec = vectorFromRawBlobValue(argv[0], &pzErrMsg);
    if (pVec == nullptr)
        sqlite3_result_error(context, pzErrMsg, -1);
    else
        resultVector(context, pVec.get());
}

static void vector_version(sqlite3_context *context,
                           int argc,
                           sqlite3_value **argv) {

    sqlite3_result_text(context, SQLITE_VSS_VERSION, -1, SQLITE_STATIC);
}

static void vector_debug(sqlite3_context *context,
                         int argc,
                         sqlite3_value **argv) {

    vec_ptr pVec = valueAsVector(argv[0]);

    if (pVec == nullptr) {

        sqlite3_result_error(context, "Value not a vector", -1);
        return;
    }

    sqlite3_str *str = sqlite3_str_new(0);
    sqlite3_str_appendf(str, "size: %lld [", pVec->size());

    for (int i = 0; i < pVec->size(); i++) {

        if (i == 0)
            sqlite3_str_appendf(str, "%f", pVec->at(i));
        else
            sqlite3_str_appendf(str, ", %f", pVec->at(i));
    }

    sqlite3_str_appendchar(str, 1, ']');
    sqlite3_result_text(context, sqlite3_str_finish(str), -1, sqlite3_free);
}

#endif // TRANSFORMERS_H
