// yoyo
package vss

// #cgo LDFLAGS: -lsqlite_vss0 -lfaiss_avx2
// #cgo linux,amd64 LDFLAGS: -lm -lgomp -latlas -lblas -llapack
// #cgo CFLAGS: -DSQLITE_CORE
// #include <sqlite3ext.h>
// #include "sqlite-vss.h"
//
import "C"

// Once called, every future new SQLite3 connection created in this process
// will have the sqlite-vss extension loaded. It will persist until [Cancel] is
// called.
//
// Calls [sqlite3_auto_extension()] under the hood.
//
// [sqlite3_auto_extension()]: https://www.sqlite.org/c3ref/auto_extension.html
func Auto() {
	C.sqlite3_auto_extension( (*[0]byte) ((C.sqlite3_vss_init)) );
}

// "Cancels" any previous calls to [Auto]. Any new SQLite3 connections created
// will not have the sqlite-vss extension loaded.
//
// Calls [sqlite3_cancel_auto_extension()] under the hood.
// [sqlite3_cancel_auto_extension()]: https://www.sqlite.org/c3ref/cancel_auto_extension.html
func Cancel() {
	C.sqlite3_cancel_auto_extension( (*[0]byte) (C.sqlite3_vss_init) );
}
