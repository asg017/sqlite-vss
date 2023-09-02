package vss

import (
	"database/sql"
	"path"

	"github.com/mattn/go-sqlite3"
)

// Open returns a sql.DB pre-configured with sqlite-vss extensions,
// to work vss0 and vector0 files should be stored under extPath.
func Open(dbName string, extPath string) (*sql.DB, error) {
	sql.Register("sqlite-vss", &sqlite3.SQLiteDriver{
		Extensions: []string{
			path.Join(extPath, "vector0"),
			path.Join(extPath, "vss0"),
		},
	})
	return sql.Open("sqlite-vss", dbName)
}
