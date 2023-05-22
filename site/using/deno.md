<script setup>
import { data } from '../project.data.ts'
const { VERSION } = data;
</script>

# `sqlite-vss` with Deno

[![deno.land/x release](https://img.shields.io/github/v/release/asg017/sqlite-vss?color=fef8d2&include_prereleases&label=deno.land%2Fx&logo=deno)](https://deno.land/x/sqlite_vss)

`sqlite-vss` is available to Deno developers with the [`x/sqlite_vss`](https://deno.land/x/sqlite_vss) Deno module. It works with [`x/sqlite3`](https://deno.land/x/sqlite3), the native Deno SQLite module.

## Quickstart

::: code-group

```ts-vue [main.ts]
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

:::

```bash-vue
deno run -A --unstable main.ts
```

## Storing Vectors
