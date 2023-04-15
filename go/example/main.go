package main

import (
	"fmt"
	"log"

	vss "github.com/asg017/sqlite-vss/go"
)

func main() {
	// /usr/local/lib/sqlite/ext/sqlite-vss is where vss0 and vector0 are stored.
	db, err := vss.Open("vss-example.db", "/usr/local/lib/sqlite/ext/sqlite-vss")
	if err != nil {
		log.Fatal()
	}
	defer db.Close()

	r := db.QueryRow("select vss_version();")
	if err := r.Err(); err != nil {
		log.Fatal(err)
	}

	var versions any
	if err := r.Scan(&versions); err != nil {
		log.Fatal(err)
	}

	fmt.Println(versions)
}
