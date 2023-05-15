defmodule SqliteVss.Extension do
  @moduledoc """
  Extension module for adding the sqlite-vss extension to an Ecto Repo.

  In order to use this add the following to the `config.exs` file.

  ```
  config :my_app, MyApp.Repo,
    after_connect: {SqliteVss.Extension, :enable, []}
  ```
  """

  def enable(_conn) do
    [db_conn] = Process.get(:"$callers")
    db_connection_state = :sys.get_state(db_conn)
    conn = db_connection_state.mod_state.state
    :ok = Exqlite.Basic.enable_load_extension(conn)

    Exqlite.Basic.load_extension(conn, SqliteVss.bin_path() <> "/vector0")

    Exqlite.Basic.load_extension(conn, SqliteVss.bin_path() <> "/vss0")
  end
end
