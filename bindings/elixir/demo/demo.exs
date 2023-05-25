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
