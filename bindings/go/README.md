# Golang bindings for `sqlite-vss`

[![Go Reference](https://pkg.go.dev/badge/github.com/asg017/sqlite-vss/bindings/go.svg)](https://pkg.go.dev/github.com/asg017/sqlite-vss/bindings/go)

## Linking `sqlite-vss` libraries

Option #1:

```go
package main

import (
	"database/sql"

	_ "github.com/asg017/sqlite-vss/bindings/go"
	_ "github.com/mattn/go-sqlite3"
)

// #cgo LDFLAGS: -Lpath/to/sqlite-vss-libs/ -Wl,-undefined,dynamic_lookup
import "C"

func main() {
  /* */
}

```

Option #2:

```bash
CGO_LDFLAGS="-Lpath/to/sqlite-vss-libs/ -Wl,-undefined,dynamic_lookup" go build .
```
