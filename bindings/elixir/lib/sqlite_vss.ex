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
  def current_version do
    config = :hex_core.default_config()

    {:ok, _} = Application.ensure_all_started(:inets)
    {:ok, _} = Application.ensure_all_started(:ssl)

    updated_config =
      Map.put(config, :http_adapter, {:hex_http_httpc, %{http_options: http_options()}})

    case :hex_api_package.get(updated_config, "sqlite_vss") do
      {:ok, {200, _headers, response}} ->
        response
        |> Map.get("releases")
        |> Enum.map(& &1["version"])
        |> List.first()

      {:ok, {404, _, _}} ->
        # TODO: remove this fallback once sqlite_vss is published
        "0.1.1-alpha.8"

      {:error, reason} ->
        reason
    end
  end

  @doc """
  Installs sqlite_vss with `current_version/0`.
  """
  def install(base_url \\ default_base_url()) do
    url = get_url(base_url)

    tar_binary = fetch_body!(url)

    cache_dir =
      :filename.basedir(
        :user_cache,
        Path.join("sqlite_vss_precompiled", "precompiled_extensions"),
        %{}
      )

    cache_tar_gz_filename = filename(current_target!())

    full_cache_path = cache_dir <> "/" <> cache_tar_gz_filename

    bin_path = bin_path()

    if File.exists?(full_cache_path) do
      with :ok <- check_file_integrity(full_cache_path),
           :ok <-
             :erl_tar.extract({:binary, tar_binary}, [
               :compressed,
               cwd: to_charlist(bin_path)
             ]) do
        Logger.debug("Copying artifact from cache and extracting to #{bin_path}")
        :ok
      end
    else
      with :ok <- File.mkdir_p(Path.dirname(bin_path)),
           :ok <- File.mkdir_p(cache_dir),
           :ok <- File.write(cache_dir, tar_binary),
           :ok <- check_file_integrity(full_cache_path),
           :ok <-
             :erl_tar.extract({:binary, tar_binary}, [
               :compressed,
               cwd: to_charlist(bin_path)
             ]) do
        :ok
      end
    end

    target_urls = Enum.map(default_targets(), &get_url(default_base_url(), &1))

    result = download_artifacts_with_checksums!(target_urls)

    checksum_file_path = Path.join(File.cwd!(), "checksum-sqlite-vss.exs")

    pairs =
      for %{path: path, checksum: checksum, checksum_algo: algo} <- result, into: %{} do
        basename = Path.basename(path)
        checksum = "#{algo}:#{checksum}"
        {basename, checksum}
      end

    lines =
      for {filename, checksum} <- Enum.sort(pairs) do
        ~s(  "#{filename}" => #{inspect(checksum, limit: :infinity)},\n)
      end

    File.write!(checksum_file_path, ["%{\n", lines, "}\n"])

    map_from_file = map_from_file(checksum_file_path)

    check_integrity_from_map(map_from_file, checksum_file_path)
  end

  def check_integrity_from_map(checksum_map, file_path) do
    basename = Path.basename(file_path)

    case Map.fetch(checksum_map, basename) do
      {:ok, algo_with_hash} ->
        [algo, hash] = String.split(algo_with_hash, ":")
        algo = String.to_existing_atom(algo)

        case File.read(file_path) do
          {:ok, content} ->
            file_hash =
              algo
              |> :crypto.hash(content)
              |> Base.encode16(case: :lower)

            if file_hash == hash do
              :ok
            else
              {:error, "the integrity check failed because the checksum of files does not match"}
            end

          {:error, reason} ->
            {:error,
             "cannot read the file for checksum comparison: #{inspect(file_path)}. " <>
               "Reason: #{inspect(reason)}"}
        end

      :error ->
        {:error,
         "the precompiled artifact does not exist in the checksum file." <>
           " Please consider running: `mix sqlite_vss.install` to generate the checksum file."}
    end
  end

  def map_from_file(checksum_file_path) do
    with {:ok, contents} <- File.read(checksum_file_path),
         {%{} = contents, _} <- Code.eval_string(contents) do
      contents
    else
      _ -> %{}
    end
  end

  def check_file_integrity(file_path) do
    checksum_file_path = Path.join(File.cwd!(), "checksum-sqlite-vss.exs")

    checksum_file_path
    |> map_from_file()
    |> check_integrity_from_map(file_path)
  end

  def download_artifacts_with_checksums!(urls) do
    cache_dir =
      :filename.basedir(
        :user_cache,
        Path.join("sqlite_vss_precompiled", "precompiled_extensions"),
        %{}
      )

    tasks = Task.async_stream(urls, fn url -> {url, fetch_body!(url)} end)
    :ok = File.mkdir_p(cache_dir)

    Enum.flat_map(tasks, fn {:ok, result} ->
      with {:download, {url, body}} <- {:download, result},
           hash <- :crypto.hash(:sha256, body),
           path <- Path.join(cache_dir, basename_from_url(url)),
           {:file, :ok} <- {:file, File.write(path, body)} do
        checksum = Base.encode16(hash, case: :lower)

        Logger.debug("Artifact cached at #{path} with checksum #{inspect(checksum)} (:sha256)")

        [
          %{
            url: url,
            path: path,
            checksum: checksum,
            checksum_algo: :sha256
          }
        ]
      else
        {context, result} ->
          raise "could not finish the download of artifacts. " <>
                  "Context: #{inspect(context)}. Reason: #{inspect(result)}"
      end
    end)
  end

  defp basename_from_url(url) do
    uri = URI.parse(url)

    uri.path
    |> String.split("/")
    |> List.last()
  end

  @doc """
  Returns the path to the executable.

  The executable may not be available if it was not yet installed.
  """
  def bin_path do
    {:ok, current_target} = current_target()
    name = "sqlite-vss-#{current_target}"

    Application.get_env(:sqlite_vss, :path) ||
      if Code.ensure_loaded?(Mix.Project) do
        Path.join(Path.dirname(Mix.Project.build_path()), name)
      else
        Path.expand("_build/#{name}")
      end
  end

  def filename(target) do
    "sqlite-vss-v#{current_version()}-loadable-#{target}.tar.gz"
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

  defp target_config do
    current_system_arch = system_arch()

    %{
      os_type: :os.type(),
      target_system: current_system_arch,
      word_size: :erlang.system_info(:wordsize)
    }
  end

  def current_target!(target_config \\ target_config()) do
    current_target(target_config)
    |> elem(1)
  end

  def current_target(target_config \\ target_config()) do
    %{os_type: os_type, target_system: target_system} = target_config
    arch_str = target_system |> Map.values() |> Enum.join("-")

    case os_type do
      {:win32, _} ->
        {:error, "sqlite-vss is not available for architecture: #{arch_str}"}

      {:unix, osname} ->
        osname = if osname == :darwin, do: "macos", else: osname

        case target_config.target_system.arch do
          "x86_64" ->
            {:ok, "#{osname}-x86_64"}

          "aarch64" ->
            {:ok, "#{osname}-aarch64"}

          _ ->
            {:error,
             "precompiled artifact is not available for this target: \"i686-unknown-linux-gnu\".\nThe available targets are:\n - linux-x86_64\n - macos-aarch64\n - macos-x86_64"}
        end
    end
  end

  defp default_targets do
    ["linux-x86_64", "macos-aarch64", "macos-x86_64"]
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

  defp get_url(base_url, target \\ current_target!()) do
    base_url
    |> String.replace("$version", current_version())
    |> String.replace("$target", target)
  end

  # Returns a map with `:arch`, `:vendor`, `:os` and maybe `:abi`.
  defp system_arch do
    base =
      :erlang.system_info(:system_architecture)
      |> List.to_string()
      |> String.split("-")

    triple_keys =
      case length(base) do
        4 ->
          [:arch, :vendor, :os, :abi]

        3 ->
          [:arch, :vendor, :os]

        _ ->
          # It's too complicated to find out, and we won't support this for now.
          []
      end

    triple_keys
    |> Enum.zip(base)
    |> Enum.into(%{})
  end
end
