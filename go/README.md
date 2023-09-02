# sqlite-vss Go package


This package wraps the [go-sqlite3](https://github.com/mattn/go-sqlite3) driver and automatically loads extensions when opening a database connection. 

```go
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
```

## Installing


```bash
go get github.com/asg017/sqlite-vss/go
```
