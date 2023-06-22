# `sqlite-vss` with Ruby

![Gem](https://img.shields.io/gem/v/sqlite-vss?color=red&logo=rubygems&logoColor=white)

::: warning
The Ruby bindings for `sqlite-vss` are still in beta and are subject to change. If you come across problems, [please comment on this Ruby tracking issue](https://github.com/asg017/sqlite-vss/issues/52).
:::

Ruby developers can use `sqlite-vss` with the [`sqlite-vss` Gem](https://rubygems.org/gems/sqlite-vss).

```bash
gem install sqlite-vss
```

You can then use `SqliteVss.load()` to load `sqlite-vss` SQL functions in a given SQLite connection.

```ruby
require 'sqlite3'
require 'sqlite_vss'

db = SQLite3::Database.new(':memory:')
db.enable_load_extension(true)
SqliteVss.load(db)
db.enable_load_extension(false)

result = db.execute('SELECT vss_version()')
puts result.first.first

```

Checkout [the API Reference](./api-reference) for all available SQL functions.

Also see _[Making SQLite extensions gem install-able](https://observablehq.com/@asg017/making-sqlite-extension-gem-installable) (June 2023)_ for more information on how gem install'able SQLite extensions work.
