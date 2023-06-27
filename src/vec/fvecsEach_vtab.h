
#ifndef FVECSEACH_VTAB_H
#define FVECSEACH_VTAB_H

struct fvecsEach_vtab : public sqlite3_vtab {
  
    fvecsEach_vtab() {

      pModule = nullptr;
      nRef = 0;
      zErrMsg = nullptr;
    }

    ~fvecsEach_vtab() {

        if (zErrMsg != nullptr) {
            sqlite3_free(zErrMsg);
        }
    }
};

#endif // FVECSEACH_VTAB_H
