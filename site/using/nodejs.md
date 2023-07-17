# `sqlite-vss` with Node.js

[![npm](https://img.shields.io/npm/v/sqlite-vss.svg?color=green&logo=nodedotjs&logoColor=white)](https://www.npmjs.com/package/sqlite-vss)

Node.js developers can use `sqlite-vss` with the [`sqlite-vss` NPM package](https://www.npmjs.com/package/sqlite-vss). It can be installed with:

```bash
npm install sqlite-vss
```

Once installed, the `sqlite-vss` package can be used with SQLite clients like [`better-sqlite3`](https://github.com/WiseLibs/better-sqlite3) and [`node-sqlite3`](https://github.com/TryGhost/node-sqlite3).

```js
import Database from "better-sqlite3";
import * as sqlite_vss from "sqlite-vss";

const db = new Database(":memory:");
sqlite_vss.load(db);

const version = db.prepare("select vss_version()").pluck().get();
console.log(version);
```

Checkout [the API Reference](./api-reference) for all available SQL functions.

Also see _[Making SQLite extensions npm install-able](https://observablehq.com/@asg017/making-sqlite-extensions-npm-installable-and-deno) (March 2023)_ for more information on how npm install'able SQLite extensions work.

## Working with Vectors in Node.js

### Vectors as JSON

If your vectors in Node.js are represented as an array of floats, you can insert them into a `vss0` table as a JSON string with `JSON.stringify()`.

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
