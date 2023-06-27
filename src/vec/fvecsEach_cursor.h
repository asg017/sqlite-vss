
#ifndef FVECSEACH_CURSOR_H
#define FVECSEACH_CURSOR_H

class fvecsEach_cursor : public sqlite3_vtab_cursor {

public:

    fvecsEach_cursor(sqlite3_vtab *pVtab) {
      
      this->pVtab = pVtab;
      iRowid = 0;
      pBlob = nullptr;
      iBlobN = 0;
      p = 0;
      iCurrentD = 0;
    }

    ~fvecsEach_cursor() {

        if (pBlob != nullptr)
            sqlite3_free(pBlob);
    }

    void * getBlob() {

        return pBlob;
    }

    void setBlob(void * blob) {

        if (pBlob != nullptr)
            sqlite3_free(pBlob);

        pBlob = blob;
    }

    sqlite3_int64 iRowid;

    // Total size of pBlob in bytes
    sqlite3_int64 iBlobN;
    sqlite3_int64 p;

    // Current dimensions
    int iCurrentD;

    // Pointer to current vector being read in
    vec_ptr pCurrentVector;

private:

    // Copy of fvecs input blob
    void *pBlob;
};

#endif // FVECSEACH_CURSOR_H
