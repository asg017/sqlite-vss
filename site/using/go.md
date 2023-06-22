# `sqlite-vss` with Go

[![Go Reference](https://pkg.go.dev/badge/github.com/asg017/sqlite-vss/bindings/go.svg)](https://pkg.go.dev/github.com/asg017/sqlite-vss/bindings/go)

::: warning
The Go bindings for `sqlite-vss` are still in beta and are subject to change. If you come across problems, [please comment on the Go tracking issue](https://github.com/asg017/sqlite-vss/issues/49).
:::

1. binary struct pack

## Installing `sqlite-vss` into Go Projects

## Working with Vectors in Go

If your vectors in Go are represented as a slice of floats, you can insert them into a `vss0` table as a JSON string with [`json.Marshal`](https://pkg.go.dev/encoding/json#Marshal):

```go
embedding := [3]float32{0.1, 0.2, 0.3}
embeddingJson, err := json.Marshal(embedding)
if err != nil {
  log.Fatal(err)
}
_, err := tx.Exec("INSERT INTO vss_demo(a) VALUES (?)", string(embeddingJson))
if err != nil {
  log.Fatal(err)
}
```
