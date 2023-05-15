defmodule Mix.Tasks.SqliteVss.Install do
    @shortdoc "Installs SqliteVss extension files"
  use Mix.Task

  @impl true
  def run(_args) do
    Application.ensure_all_started(:sqlite_vss)

    SqliteVss.install()
  end
end
