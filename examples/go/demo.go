package main

import (
	"database/sql"
	"fmt"
	"log"

	_ "github.com/asg017/sqlite-vss/bindings/go"
	_ "github.com/mattn/go-sqlite3"
)

// #cgo LDFLAGS: -L../../dist/debug -Wl,-undefined,dynamic_lookup -lstdc++
import "C"

func main() {
	db, err := sql.Open("sqlite3", ":memory:")
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	var version string
	var vec string
	err = db.QueryRow("select vss_version(), vector_to_json(X'00002842')").Scan(&version, &vec)
	if err != nil {
		log.Fatal(err)
	}

	fmt.Printf("version result: %s\n", version)
	fmt.Printf("vec result: %s\n", vec)
}
