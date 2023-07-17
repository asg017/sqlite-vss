<script setup>
import { data } from '../project.data.ts'
const { VERSION } = data;
</script>

# `sqlite-vss` with Deno

[![deno.land/x release](https://img.shields.io/github/v/release/asg017/sqlite-vss?color=fef8d2&include_prereleases&label=deno.land%2Fx&logo=deno)](https://deno.land/x/sqlite_vss)

`sqlite-vss` is available to Deno developers with the [`x/sqlite_vss`](https://deno.land/x/sqlite_vss) Deno module. It works with [`x/sqlite3`](https://deno.land/x/sqlite3), the native Deno SQLite module.

::: code-group

```ts-vue [main.ts]
import { Database } from "https://deno.land/x/sqlite3@0.8.0/mod.ts";
import * as sqlite_vss from "https://deno.land/x/sqlite_vss@v{{ VERSION }}/mod.ts";

const db = new Database(":memory:");
db.enableLoadExtension = true;
sqlite_vss.load(db);
db.enableLoadExtension = false;

const [version] = db
  .prepare("select vss_version()")
  .value<[string]>()!;

console.log(version);
```

:::

```bash-vue
deno run -A --unstable main.ts
```

Checkout [the API Reference](./api-reference) for all available SQL functions.

Also see _[Putting SQLite extensions on `deno.land/x`](https://observablehq.com/@asg017/making-sqlite-extensions-npm-installable-and-deno) (March 2023)_ for more information on how Deno SQLite extensions work.

## Working with Vectors in Deno

### Vectors as JSON

If your vectors in Deno are represented as an array of floats, you can insert them into a `vss0` table as a JSON string with `JSON.stringify()`.

```js
const embedding = [0.1, 0.2, 0.3];
const stmt = db.prepare("INSERT INTO vss_demo VALUES (?)");
stmt.run(JSON.stringify(embedding));
```

### Vectors as Bytes

Alternatively, if your vectors in Node.js are represented as a [Float32Array](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Float32Array), use the [`.buffer`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray/buffer) accessor to insert the underlying ArrayBuffer.

```js
const embedding = new Float32Array([0.1, 0.2, 0.3]);
const stmt = db.prepare("INSERT INTO vss_demo VALUES (?)");
stmt.run(embedding.buffer);
```
