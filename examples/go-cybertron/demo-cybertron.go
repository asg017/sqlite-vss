// Copyright 2022 The NLP Odyssey Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

import (
	"context"
	"database/sql"
	"encoding/json"
	"log"
	"os"

	"github.com/nlpodyssey/cybertron/pkg/models/bert"
	"github.com/nlpodyssey/cybertron/pkg/tasks"
	"github.com/nlpodyssey/cybertron/pkg/tasks/textencoding"
	"github.com/rs/zerolog"

	_ "github.com/asg017/sqlite-vss/bindings/go"
	sqlite "github.com/mattn/go-sqlite3"
)

// #cgo linux,amd64 LDFLAGS: -L../../dist/debug -Wl,-undefined,dynamic_lookup -lstdc++
// #cgo darwin,amd64 LDFLAGS: -L../../dist/debug -Wl,-undefined,dynamic_lookup -lomp
import "C"

func main() {
	zerolog.SetGlobalLevel(zerolog.DebugLevel)

	// load the sentence-transformers model and define a new SQL scalar function
	// "st_encode" that creates a JSON representation of the embedding of a given text
	modelsDir := os.Getenv("CYBERTRON_MODELS_DIR")
	modelName := "sentence-transformers/all-MiniLM-L6-v2"
	m, err := tasks.Load[textencoding.Interface](&tasks.Config{ModelsDir: modelsDir, ModelName: modelName})
	if err != nil {
		log.Fatal(err)
	}
	defer tasks.Finalize(m)
	st_encode := func(text string) string {
		result, err := m.Encode(context.Background(), text, int(bert.MeanPooling))
		if err != nil {
			log.Fatal(err)
		}
		vec := result.Vector.Data().F32()
		vecBytes, err := json.Marshal(vec)
		if err != nil {
			panic(err)
		}
		return string(vecBytes)
	}

	sql.Register("sqlite3_custom", &sqlite.SQLiteDriver{
		ConnectHook: func(conn *sqlite.SQLiteConn) error {
			if err := conn.RegisterFunc("st_encode", st_encode, true); err != nil {
				return err
			}
			return nil
		},
	})

	db, err := sql.Open("sqlite3_custom", "tmp.db")
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	// initialize fruits and vss_fruits tables
	_, err = db.Exec("CREATE TABLE IF NOT EXISTS fruits (id INTEGER PRIMARY KEY AUTOINCREMENT, name text)")
	if err != nil {
		log.Fatal(err)
	}
	// 384 == number of dimensions that the model outputs
	_, err = db.Exec("CREATE VIRTUAL TABLE IF NOT EXISTS vss_fruits USING vss0(embedding(384))")
	if err != nil {
		log.Fatal(err)
	}

	tx, err := db.Begin()
	if err != nil {
		log.Fatal(err)
	}

	// insert these fruits into the fruits table, then generate
	// embeddings for each fruit and store those vectors in vss_fruits
	fruits := []string{"apples", "bananas", "broccoli"}
	for _, fruit := range fruits {
		f, err := tx.Exec("INSERT INTO fruits (name) VALUES (?)", fruit)
		if err != nil {
			log.Fatal(err)
		}
		id, err := f.LastInsertId()
		if err != nil {
			log.Fatal(err)
		}

		_, err = tx.Exec("INSERT INTO vss_fruits(rowid, embedding) values (?, st_encode(?))", id, fruit)
		if err != nil {
			panic(err)
		}
	}
	tx.Commit()

	rows, err := db.Query(`
            with similar_matches as (
              select rowid, vss_distance_linf(embedding, st_encode(?1)) distance
              from vss_fruits
	          where vss_search(embedding, st_encode(?1))
              limit 20
            ), final as (
              select
                fruits.rowid,
                fruits.name,
                similar_matches.distance
              from similar_matches
              left join fruits on fruits.rowid = similar_matches.rowid
              group by fruits.name
            )
            select rowid, name, distance from final order by distance`,
		"red")
	if err != nil {
		log.Fatal("vss_search query error:", err)
	}
	defer rows.Close()

	for rows.Next() {
		var rowid int64
		var name string
		var distance float64
		if err := rows.Scan(&rowid, &name, &distance); err != nil {
			log.Fatal(err)
		}
		log.Printf("rowid=%d name=%s distance=%f\n", rowid, name, distance)
	}
	if err := rows.Err(); err != nil {
		log.Fatal(err)
	}
}
