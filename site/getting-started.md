---
---

# Getting Started

## Introduction

## Installing

You have several options to include `sqlite-vss` into your projects, including PyPi packages for Python, NPM packages for Node.js, Gems for Ruby, and more.

::: code-group

```bash [Python]
pip install sqlite-vss
```

```bash [Node.js]
npm install sqlite-vss
```

```js [Deno]
import * as sqlite_vss from "https://deno.land/x/sqlite_vss@v0.1.0/mod.ts";
```

```bash [Ruby]
gem install sqlite-vss
```

```bash [Datasette]
datasette install datasette-sqlite-vss
```

```bash [Go]
go get github.com/asg017/sqlite-vss/go
```

```bash [Rust]
cargo add sqlite-vss
```

```bash [sqlite-package-manager]
spm install github.com/asg017/sqlite-vss
```

:::

For quick

## Basic Example

<!-- <p align="center"> <img src="./demo_base.png" width="75%"> </p> -->

<p align="center"> <img src="./demo_base_dark.png" width="75%"> </p>

```sqlite
create virtual table vss_lookup using vss0(
  a(2)
);

insert into vss_lookup(rowid, a)
  select
    key as rowid,
    value as a
  from json_each('
    [
      [1.0, 3.0],
      [3.0, 1.0],
      [-2.0, -2.0],
      [-4.0, 1.0]
    ]
  ');


```

<p align="center"> <img src="./demo_q1.png" width="75%"> </p>

```sqlite

select
  rowid,
  distance
from vss_lookup
where vss_search(a, json('[2.0, 2.0]'))
limit 3;

```

```
┌───────┬──────────┐
│ rowid │ distance │
├───────┼──────────┤
│ 1     │ 2.0      │
│ 2     │ 2.0      │
│ 3     │ 32.0     │
└───────┴──────────┘
```

<p align="center"> <img src="./demo_q2.png" width="75%"> </p>

```sqlite

select
  rowid,
  distance
from vss_lookup
where vss_search(a, json('[-2.0, -2.0]'))
limit 3;

```

```
┌───────┬──────────┐
│ rowid │ distance │
├───────┼──────────┤
│ 3     │ 0.0      │
│ 4     │ 13.0     │
│ 1     │ 34.0     │
└───────┴──────────┘
```

## Next Steps
