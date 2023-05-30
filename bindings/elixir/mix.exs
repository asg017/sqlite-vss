defmodule SqliteVss.MixProject do
  use Mix.Project

  @source_url "https://github.com/asg017/sqlite-vss/bindings/elixir"

  def project do
    [
      app: :sqlite_vss,
      version: "0.1.0",
      elixir: "~> 1.14",
      start_permanent: Mix.env() == :prod,
      deps: deps(),
      name: "sqlite_vss",
      source_url: "https://github.com/asg017/sqlite-vss",
      homepage_url: "https://github.com/asg017/sqlite-vss",
      docs: [
        main: "SqliteVss",
        extras: ["README.md"],
        source_ref: "v0.1.0"
      ],
      package: package()
    ]
  end

  def application do
    [
      extra_applications: [:logger, inets: :optional, ssl: :optional],
      mod: {SqliteVss, []},
      env: [default: []]
    ]
  end

  defp deps do
    [
      {:ex_doc, "~> 0.14", only: :dev, runtime: false},
      {:ecto_sqlite3, ">= 0.0.0"},
      {:castore, ">= 0.0.0"},
      {:hex_core, "~> 0.10.0"}
    ]
  end

  defp package do
    [
      files: [
        "lib",
        "mix.exs",
        "README.md",
      ],
      links: %{"GitHub" => @source_url},
      maintainers: ["Alex Garcia", "Tommy Rodriguez"]
    ]
  end
end
