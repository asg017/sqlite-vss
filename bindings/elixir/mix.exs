defmodule SqliteVss.MixProject do
  use Mix.Project

  @source_url "https://github.com/asg017/sqlite-vss/tree/main/bindings/elixir"
  @version File.read!(Path.expand("./VERSION", __DIR__)) |> String.trim()

  def project do
    [
      app: :sqlite_vss,
      version: @version,
      elixir: "~> 1.14",
      start_permanent: Mix.env() == :prod,
      deps: deps(),
      name: "sqlite_vss",
      source_url: @source_url,
      homepage_url: @source_url,
      docs: [
        main: "SqliteVss",
        extras: ["README.md"],
        source_ref: "v" <> @version
      ],
      description: description(),
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

  defp description() do
    "sqlite-vss please."
  end

  defp package do
    [
      files: [
        "VERSION",
        "lib",
        "mix.exs",
        "README.md",
        "sqlite-vss-checksum.exs"
      ],
      links: %{"GitHub" => @source_url},
      maintainers: ["Alex Garcia", "Tommy Rodriguez"],
      licenses: ["MIT"],
    ]
  end
end
