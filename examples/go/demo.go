package main

import (
	"database/sql"
	"fmt"
	"log"

	_ "github.com/asg017/sqlite-vss/bindings/go"
	_ "github.com/mattn/go-sqlite3"
)

// #cgo linux,amd64 LDFLAGS: -Wl,-undefined,dynamic_lookup -lstdc++
// #cgo darwin,amd64 LDFLAGS: -Wl,-undefined,dynamic_lookup -lomp
// #cgo darwin,arm64 LDFLAGS: -Wl,-undefined,dynamic_lookup -lomp
import "C"

func main() {
	db, err := sql.Open("sqlite3", ":memory:")
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	var version, vector string
	err = db.QueryRow("SELECT vss_version(), vector_to_json(?)", []byte{0x00, 0x00, 0x28, 0x42}).Scan(&version, &vector)
	if err != nil {
		log.Fatal(err)
	}

	fmt.Printf("version=%s vector=%s\n", version, vector)

	_, err = db.Exec(`
	CREATE VIRTUAL TABLE vss_demo USING vss0(a(2));
    INSERT INTO vss_demo(rowid, a)
      VALUES
          (1, '[1.0, 2.0]'),
          (2, '[2.0, 2.0]'),
          (3, '[3.0, 2.0]')
	`)
	if err != nil {
		log.Fatal(err)
	}

	rows, err := db.Query(`
		SELECT
			rowid,
			distance
		FROM vss_demo
		WHERE vss_search(a, '[1.0, 2.0]')
		LIMIT 3
	`)
	if err != nil {
		log.Fatal(err)
	}

	for rows.Next() {
		var rowid int64
		var distance float32
		err = rows.Scan(&rowid, &distance)
		if err != nil {
			log.Fatal(err)
		}
		fmt.Printf("rowid=%d, distance=%f\n", rowid, distance)
	}
	err = rows.Err()
	if err != nil {
		log.Fatal(err)
	}

	fmt.Println("✅ demo.go ran successfully. \n");
}
