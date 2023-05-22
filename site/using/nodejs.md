# `sqlite-vss` with Node.js

[![npm](https://img.shields.io/npm/v/sqlite-vss.svg?color=green&logo=nodedotjs&logoColor=white)](https://www.npmjs.com/package/sqlite-vss)

Node.js developers can use `sqlite-vss` with the [`sqlite-vss` NPM package](https://www.npmjs.com/package/sqlite-vss):

```bash
npm install sqlite-vss
```

## Quickstart

Once installed, the `sqlite-vss` package can be used with SQLite clients like [`better-sqlite3`](https://github.com/WiseLibs/better-sqlite3) and [`node-sqlite3`](https://github.com/TryGhost/node-sqlite3).

```js
import Database from "better-sqlite3";
import * as sqlite_vss from "sqlite-vss";

const db = new Database(":memory:");
sqlite_vss.load(db);

const version = db.prepare("select vss_version()").pluck().get();
console.log(version);
```
