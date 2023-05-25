defmodule SqliteVss do
  @moduledoc """
  `SqliteVss` module which installs the vector0 and vss0 extensions.
  """
  use Application
  require Logger

  @doc false
  def start(_, _) do
    Supervisor.start_link([], strategy: :one_for_one)
  end

  @doc """
  Returns the configured sqlite_vss version.
  """
  def configured_version do
    config = :hex_core.default_config()

    updated_config =
      Map.put(config, :http_adapter, {:hex_http_httpc, %{http_options: http_options()}})

    case :hex_api_package.get(updated_config, "sqlite_vss") do
      {:ok, {200, _headers, response}} ->
        response
        |> Map.get("releases")
        |> Enum.map(& &1["version"])
        |> List.first()

      {:ok, {404, _, _}} ->
        #https://github.com/asg017/sqlite-vss/releases/download/v0.1.1-alpha.7/sqlite-vss-v0.1.1-alpha.7-loadable-macos-aarch64.tar.gz
        # TODO: remove this fallback once sqlite_vss is published
        "0.1.1-alpha.7"

      {:error, reason} ->
        IO.inspect(reason, label: "Something went wrong")
    end
  end

  @doc """
  Installs sqlite_vss with `configured_version/0`.
  """
  def install(base_url \\ default_base_url()) do
    tmp_dir =
      freshdir_p(Path.join(System.tmp_dir!(), "sqlite_vss")) ||
        raise "could not install sqlite_vss."

    url = get_url(base_url)

    IO.inspect(url, label: "line_number: 49, filename: sqlite_vss.ex")
    tar = fetch_body!(url)

    bin_path = bin_path()
    File.mkdir_p!(Path.dirname(bin_path))

    case :erl_tar.extract({:binary, tar}, [:verbose, :compressed, cwd: to_charlist(tmp_dir)]) do
      :ok -> :ok
      other -> raise "couldn't unpack archive: #{inspect(other)}"
    end

    File.cp_r!(Path.join([tmp_dir]), bin_path)
  end

  @doc """
  Returns the path to the executable.

  The executable may not be available if it was not yet installed.
  """
  def bin_path do
    name = "sqlite-vss-#{target()}"

    Application.get_env(:sqlite_vss, :path) ||
      if Code.ensure_loaded?(Mix.Project) do
        Path.join(Path.dirname(Mix.Project.build_path()), name)
      else
        Path.expand("_build/#{name}")
      end
  end

  defp freshdir_p(path) do
    with {:ok, _} <- File.rm_rf(path),
         :ok <- File.mkdir_p(path) do
      path
    else
      _ -> nil
    end
  end

  defp target do
    arch_str = :erlang.system_info(:system_architecture)

    case :os.type() do
      {:win32, _} ->
        raise "sqlite-vss is not available for architecture: #{arch_str}"

      {:unix, osname} ->
        [arch | _] = arch_str |> List.to_string() |> String.split("-")

        osname =
          if osname == :darwin, do: "macos", else: osname

        case arch do
          "x86_64" -> "#{osname}-x86_64"
          "aarch64" -> "#{osname}-aarch64"
          _ -> raise "sqlite-vss is not available for architecture: #{arch_str}"
        end
    end
  end

  defp http_options(scheme) do
    http_options()
    |> maybe_add_proxy_auth(scheme)
  end

  defp http_options do
    # https://erlef.github.io/security-wg/secure_coding_and_deployment_hardening/inets
    cacertfile = cacertfile() |> String.to_charlist()

    [
      ssl: [
        verify: :verify_peer,
        cacertfile: cacertfile,
        depth: 2,
        customize_hostname_check: [
          match_fun: :public_key.pkix_verify_hostname_match_fun(:https)
        ]
      ]
    ]
  end

  defp fetch_body!(url) do
    scheme = URI.parse(url).scheme
    url = String.to_charlist(url)
    Logger.debug("Downloading sqlite-vss from #{url}")

    {:ok, _} = Application.ensure_all_started(:inets)
    {:ok, _} = Application.ensure_all_started(:ssl)

    if proxy = proxy_for_scheme(scheme) do
      %{host: host, port: port} = URI.parse(proxy)
      Logger.debug("Using #{String.upcase(scheme)}_PROXY: #{proxy}")
      set_option = if "https" == scheme, do: :https_proxy, else: :proxy
      :httpc.set_options([{set_option, {{String.to_charlist(host), port}, []}}])
    end

    options = [body_format: :binary]

    case :httpc.request(:get, {url, []}, http_options(scheme), options) do
      {:ok, {{_, 200, _}, _headers, body}} ->
        body

      other ->
        raise """
        couldn't fetch #{url}: #{inspect(other)}

        You may also install the "sqlite_vss" executable manually, \
        see the docs: https://hexdocs.pm/sqlite_vss
        """
    end
  end

  defp proxy_for_scheme("http") do
    System.get_env("HTTP_PROXY") || System.get_env("http_proxy")
  end

  defp proxy_for_scheme("https") do
    System.get_env("HTTPS_PROXY") || System.get_env("https_proxy")
  end

  defp maybe_add_proxy_auth(http_options, scheme) do
    case proxy_auth(scheme) do
      nil -> http_options
      auth -> [{:proxy_auth, auth} | http_options]
    end
  end

  defp proxy_auth(scheme) do
    with proxy when is_binary(proxy) <- proxy_for_scheme(scheme),
         %{userinfo: userinfo} when is_binary(userinfo) <- URI.parse(proxy),
         [username, password] <- String.split(userinfo, ":") do
      {String.to_charlist(username), String.to_charlist(password)}
    else
      _ -> nil
    end
  end

  defp cacertfile() do
    Application.get_env(:sqlite_vss, :cacerts_path) || CAStore.file_path()
  end

  @doc """
  The default URL to install SqliteVss from.
  """
  def default_base_url do
    "https://github.com/asg017/sqlite-vss/releases/download/v$version/sqlite-vss-v$version-loadable-$target.tar.gz"
  end

  def loadable_path_vector0() do
    SqliteVss.bin_path() <> "/vector0"
  end

  def loadable_path_vss0() do
    SqliteVss.bin_path() <> "/vss0"
  end

  defp get_url(base_url) do
    base_url
    |> String.replace("$version", configured_version())
    |> String.replace("$target", target())
  end
end
