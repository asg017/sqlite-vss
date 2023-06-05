# SqliteVss Hex Package
sqlite_vss is distributed on hex for Elixir developers.

## Installation

If [available in Hex](https://hex.pm/docs/publish), the package can be installed
by adding `sqlite_vss` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:sqlite_vss, "~> 0.0.0"}
  ]
end
```

Now you can install sqlite_vss by running:

`$ mix sqlite_vss.install`

Documentation can be generated with [ExDoc](https://github.com/elixir-lang/ex_doc)
and published on [HexDocs](https://hexdocs.pm). Once published, the docs can
be found at <https://hexdocs.pm/sqlite_vss>.

The `sqlite-vss` package is meant to be used with Exqlite like the following:

```
Mix.install([
  {:sqlite_vss, path: "../"},
  {:exqlite, "~> 0.13.0"}
], verbose: true)

Mix.Task.run("sqlite_vss.install")

alias Exqlite.Basic

{:ok, conn} = Basic.open("example.db")

:ok = Exqlite.Basic.enable_load_extension(conn)
Exqlite.Basic.load_extension(conn, SqliteVss.loadable_path_vector0())
Exqlite.Basic.load_extension(conn, SqliteVss.loadable_path_vss0())

{:ok, [[version]], [_]} = Basic.exec(conn, "select vss_version()") |> Basic.rows()

IO.puts("version: #{version}")
```

To load the extension files for an Ecto Repo add the following to your runtime.exs config.

```
# global
  config :exqlite, load_extensions: [SqliteVss.loadable_path_vector0(), SqliteVss.loadable_path_vss0()]

# per connection in a Phoenix app
config :my_app, MyApp.Repo,
  load_extensions: [SqliteVss.loadable_path_vector0(), SqliteVss.loadable_path_vss0()]
```

## Running the demo

```
cd demo

elixir demo.exs
```
