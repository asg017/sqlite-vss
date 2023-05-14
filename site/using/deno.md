<script setup>
import { data } from '../project.data.ts'
const { VERSION } = data;
</script>

# `sqlite-vss` with Deno

1. Float32Array.buffer
2. JSON -> Float32Array

[![deno version](https://deno.land/badge/sqlite_vss/version?color=fef8d2)](https://deno.land/x/sqlite_vss) [![Doc](https://doc.deno.land/badge.svg)](https://doc.deno.land/https/deno.land/x/sqlite_vss/mod.ts)

The [`sqlite-vss`](https://github.com/asg017/sqlite-vss) SQLite extension is available to Deno developers with the [`x/sqlite_vss`](https://deno.land/x/sqlite_vss) Deno module. It works with [`x/sqlite3`](https://deno.land/x/sqlite3), the fastest and native Deno SQLite3 module.

```ts-vue
import { Database } from "https://deno.land/x/sqlite3@0.8.0/mod.ts";
import * as sqlite_vss from "https://deno.land/x/sqlite_vss@v{{ VERSION }}/mod.ts";

const db = new Database(":memory:");
db.enableLoadExtension = true;
sqlite_vss.load(db);

const [version] = db
  .prepare("select vss_version()")
  .value<[string]>()!;

console.log(version);

```

Like `x/sqlite3`, `x/sqlite_vss` requires network and filesystem permissions to download and cache the pre-compiled SQLite extension for your machine. Though `x/sqlite3` already requires `--allow-ffi` and `--unstable`, so you might as well use `--allow-all`/`-A`.

```bash
deno run -A --unstable <file>
```

`x/sqlite_vss` does not work with [`x/sqlite`](https://deno.land/x/sqlite@v3.7.0), which is a WASM-based Deno SQLite module that does not support loading extensions.
