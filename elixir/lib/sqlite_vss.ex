defmodule SqliteVss do
  @moduledoc """
  Documentation for `SqliteVss`.
  """
alias ElixirSense.Plugins.Ecto

  @latest_version "0.1.1-alpha.1"

  use Application
  require Logger

  import Ecto.Query

  @doc """
  Create a vector similarity search query.

  ## Examples

      iex> SqliteVss.search_query(query, "vss_posts", "title_embedding", <<1.0>>)
      #Ecto.Query<from p0 in SoftJobs.Jobs.Post,
       join: v1 in subquery(from v0 in "vss_posts",
       where: fragment(
       "vss_search(?, ?)",
       v0.title_embedding,
       type(
       ^"\x06\xD6r<\xD2\xF0T\xBB7\xB5d\xBD\x1F\x9FU\xBD\x92\xDCü\xAA\xB9\xC0\xBD\xAC]W=\xBB\xECV=\x9D\x19$\xBD\x9Bp\x88=\x9C\x19\x18\xBD\xEF\xB7\xF7;\xEF\xF1\xAA\xBBZ'w\xBC\xB3\x02\xA0\xBD\xDBEy\xBC%/\xA2\xBCŷ\x8C\xBD\xBCԡ<nT\xE1\xBD\r\x96\x02=F/Y<<ğ\xBD\x19\xC9\xE2<\xC2\xD9\x03\xBD\\\x01\x1A<\x18u\x96=\xEF[4\xBB\x18\xA6\xB6\xBC8\x82\xC0\xBD\xB3\xA9\xAA\xBC\x9F#j\xBB\nu`=\xED\x15\x9B\xBB~\xC9J=\xFF7\xF5<\xB7\x04\xC9=\xBD\x94\x82\xBD\xE3x\x05\xBC\xCE\xEB/\xBD\x84\x9Dٽ\xBF\xC24<\xB0\x83==\xA9\xC9\xE5<N\xEB5\xBB<k\xD0;/3\xA1\xBC\x91\x0E\x17\xBDu\xE6\xFC<\xA0\x1C\x12<\xECx\xA8\xBD^\x91\x86\xBD,\x811=E@ȽE\x1A\x8E=\x1C\xFEi=\xD8\xE7\xD4<\x86\xA5\a=vBܽ\xA2\0Ž |4\xBD\e\x02\x02=\xF2\xB5S\xBDB8\r=ׅX\xBD\x1C\x13i=\xE4\x88\f\xBDkE2=\x15\eG=~s\xB5\xBD\xD2X\xB3\xBD$\xE3)\xBC\xE5?\xD7<)E,=\xEA=\x03=\x02\x9F\b=\xA5\x8C\x12=\x10g\x17\xBC\xF1\xE4\a\xBC\xFF\xB6\t;\x82m\xB5=\x13\xF8Ҽr\xD8O\xBE\xE2N\x80=\xA6\xE6\xFA<\xB8\xAF\"\xBC\xD7\x9F\xBDa^\xA7\xBAq$\xD4<\x81\x83\r=Tp\d=\xB8C\x1D\xBD\xF9\xD8\x03>\xBB\xAEټ\xC9\x05\xBF<:\xED\x87<\xE9\xC4\x12=sI\x9F\xBD\xA0\xB0s<T\x97+=\xF26\x88<\x93Pt\xBB`\xAF\xD0=\xDA7\xAC=\a\xA4\xB5\xBD\x87uK=\x1C\x902=<6\x9A;j\xD4f=N\x1A꽩1}\xBD*\d\x13<Q\x88\xF8\xBC$=\xF3\xBB\xB5\xEFq<e\xBB\xFB\xBC\xF2Ԁ\xBCd\a\x91<\xCFG\xEF<\x06捼\x9E\a\xC0\xBB\x92\x99\xF8\xBC\x99\x82\x80=\xB7\x98.=\xB5^\xB8<k\xBF\x87=#\x9C);\"؂\x89\xD6R\x82\xBD\x95\xAB*=h\xD82\xBD@\xA2Q=J\xF1\xC1\xBC=M8\xBDB\x11\xEC9\x1E\x9E\x17=\x8C7e=|\xE8C=\xC2ܰ\xBD:\xDD\xEE:\xBE\xB7Z\xBC\xFC\x84\xE9\xBCZ\xC9\xDC=\x18\xBC\x81<A <\x14b\x8E;&0\x12\xBD\xB9\x8A2=uM⼪\x98\x87\xBD\xD5Uo\xBD\x85\xB9\xBC\xC6\x16\x1D>\xD5K\x05\xBD\b\x15w;h\xF7\xEF\xBB/K\x11\xBD\xA9F[;\xD9I#\xBC\r\x02\xDC=\xE9Xʺ\xA3\xC9A<\"mg\xBD\xA0\x1D\xF3<\x95\xBD\xF67\x91\x8Bb\xBD}\xC3E=\x17\xE0\xFA\xBB\xD2k\x90\xBCI\xB8˹\v\xCB\xC7<ܲq;8Jz\xBC\x16P\f<\x14\xCD%\xBD\a\x86\x11=\xED\xC2b=\xB3\eU<l\x01+\xBD\xAB_\x16=2\x01\xD2=&\x80\x18\xBDb\xEA\x9B=y\x89\x15\xBD*\x84\xBE=\xBB07\xBDm\xE4d=\x91k\x17>U\x10v\xBD\xA9\xB9\x99\xBB\x80\xD7\x12<\xF6p\x9F\xBB\xAA\x02\x8D=I\xD2\x03=\xA1\x9A~<\xE36M:\x99\x90{=\x93h?=\xB5\x9C6\xBE{}\xCD;\r\d\x80=\xE4z\xE5\xBC\x1A+\xC3<\x8E\"\n\xBD\xB1q\xA6\xBC\xA6\xC0)=\xCB\x14\xAC;k\xABD=#\v\xDE;=\"h=\xE49\xF1<I\xE3\a=\x13\x9A\0>\xCD75\xBD\xAFu\xAB\xBDg\xCD-=u@ż\x8A>м\x92\xEBؼ\xBF\x94\x96\xBC\x95\xE0ּ*\xCD\xF7=\xE7\xA1Y;y\v\x04\t\x80\xB4\xEB\xBC\xD4SE\xBCql\x93\xBD\x04jz<\xCE\x0E\xB4;)\xAB\xA3\xBC\xB4\x8CU<[zӻ\xC1\x9F\xA5\xBCj:\x88<\x1A\xE2D\xBD\xE69\x9B;\b\x8ET=Z%*<f)R;\xAE;D\xBCI\xA4\x96\xBD֞ɽX\x8F\x8E<,օ;\xA50\xB3<b\xAB\xB4=!\xEBj\xBD\xE9Hн\x8D\x04\x9E\xBC\x05i\xF9;\xC0\xF8\xC8<V\v\xD7< \x82>\xBD\xA7\x9B\xC1;\x89\xD7[={\x92\x0E\xBC\xDFT\x1A=\xAA\xAB\xFC<\x8DP\x98\xBC3\xD5Ǽ\xCF\xEDn=\x92D\xA8=\x8B`U\xBC\x18g\x93\xBC\x99\x8D\x0F=-g\xBA\xBCM1b<\xF5ҝ\xBD\\\xE0\xE6<\xB0z\xA2<n\xEB\e< \x8D\xDB;\xDFsc\xBB\x9C̽\e\x11}\xBDG%/\xBD\xA5.&\xBC\xAE\xB9S<\x9DΩ=\xA58y\xBC\x01\xF7\x0F=\xC0\xBEM;\xBEW\r=3\xAB\e\xBD\xB4<\f\xBDo\x8E\xE3\xBD\xD3\v&=~\x80\xAB<\xD5b\xEA\xBCg\xEBg\xBCR\x17\xB3=\xBCc\xE2<\xA5\x9CȼL\xA5>=\x18\xFFD=\xCB\xDD\xE3<\x04Rl=\xCD\xFB\x9D=\xA5\xE6\x84<[K\xC0\xBD\xF8\x06Ľ\xC5⋼x\xC6\x18\xBA\x01^\x92;\x05\x84\xA7\xBC\xF7h\x84\xBD\xD9Mc\xBC\x1A!\xCD=Q\xAA:\xBD'\xAF:=(ĩ\xBB\x05,%<ZH\xD9<;\xB3\xF7\xBD\x0F\xF0ϼ\x96ʭ\xBC\xA3L\x9C;\xBF\xE3w\xBB\xF9\xE7Լ\x9F\rc\xB2Dߋ\xBC\x1F2\xD9;\x86\\5\xBC\x19qB<뼿\xBD\xA3T\x89=\xFE\xC1I\xBD\x03\xD7c\xBD馻\xBBo\xA7\x9B\xBDщ\x88;bT`\xBD\x1AÑ\xBB\xBD\x81==''\xB4<\xC7g\xEF<\x93C\x9E\xBCm\xE3\xC1=\xB3\xE1,\xBD\xE5\xF1u\xBDnT\xA5=u;\e=\xB4Q\xFD<c\x03W=c\\\xEE;\xF8\r\xF7\xBC\xEF\xD3\x19\xBDR\xC6r\xBDǳ\x80=\aX\\=>\xBA\x9D\xBD\x81\xFD\r=x}\xC4;\xAA\xBF4=)]\x8D=\xA6\x92b\xBD\xB2&\x97<\xA7Q\xBA\xBC\xD9\x12D=ڳ\xFB<J9i\xBD\xE8\xC9\x17=\xB4\xDF$\xBD\x10>\b<vӹ=\x164Ӹ\xA7\xE1˼\da'\xBC\xA5\xA6D\xBD\x9A\x01U\xBD\x03\fm=},\x0F=\xB2ⅽ\xE5t\x06>\xEA(\xA5<9\xC9\xE1\xBBm\xF0\d=\x9CA\x99\xBD\xAB\f\xB1<\x0E\xE2\x10=\xA1@\x16\xBD\x85\x9F\v=G\xFA>\xBD#\xFE\xF2;",
       :binary
     )
   ),
   order_by: [asc: v0.distance],
   limit: ^100,
   select: [:rowid, :distance]),
   on: v1.rowid == p0.id>

  """
  @spec search_query(Ecto.Query.t(), String.t(), String.t(), binary(), integer() | none()) :: Ecto.Query.t()
  def search(query, table_name, embedding_column_name, vector, limit \\ 100) do
    subquery =
      from(s in table_name,
        where:
          fragment(
            "vss_search(?, ?)",
            field(s, ^String.to_existing_atom(embedding_column_name)),
            type(^vector, :binary)
          ),
        select: [:rowid, :distance],
        order_by: s.distance,
        limit: ^limit
      )

    from(p in query,
      join: s in subquery(subquery),
      on: s.rowid == p.id
    )
  end

  @doc false
  def start(_, _) do
    unless Application.get_env(:sqlite_vss, :version) do
      Logger.warn("""
      sqlite_vss version is not configured. Please set it in your config files:

          config :sqlite_vss, :version, "#{latest_version()}"
      """)
    end

    Supervisor.start_link([], strategy: :one_for_one)
  end

  @doc """
  Returns the configured sqlite_vss version.
  """
  def configured_version do
    Application.get_env(:sqlite_vss, :version, latest_version())
  end

  @doc """
  Installs sqlite_vss with `configured_version/0`.
  """
  def install(base_url \\ default_base_url()) do
    tmp_dir =
      freshdir_p(Path.join(System.tmp_dir!(), "sqlite_vss")) ||
        raise "could not install sqlite_vss."

    urls = for file <- ["vector0", "vss0"], do: get_url(base_url, file)

    tars = for url <- urls, do: fetch_body!(url)

    for tar <- tars do
      case :erl_tar.extract({:binary, tar}, [:compressed, cwd: to_charlist(tmp_dir)]) do
        :ok -> :ok
        other -> raise "couldn't unpack archive: #{inspect(other)}"
      end
    end

    bin_path = bin_path()
    File.mkdir_p!(Path.dirname(bin_path))

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

  @doc false
  # Latest known version at the time of publishing.
  def latest_version, do: @latest_version

  # Available targets:
  # sqlite-vss-v0.1.1-alpha.1-vector0-darwin-aarch64.tar.gz
  # sqlite-vss-v0.1.1-alpha.1-vector0-darwin-x86_64.tar.gz
  # sqlite-vss-v0.1.1-alpha.1-vector0-linux-x86_64.tar.gz
  # sqlite-vss-v0.1.1-alpha.1-vss0-darwin-aarch64.tar.gz
  # sqlite-vss-v0.1.1-alpha.1-vss0-darwin-x86_64.tar.gz
  # sqlite-vss-v0.1.1-alpha.1-vss0-linux-x86_64.tar.gz
  defp target do
    arch_str = :erlang.system_info(:system_architecture)

    case :os.type() do
      {:win32, _} ->
        raise "sqlite-vss is not available for architecture: #{arch_str}"

      {:unix, osname} ->
        [arch | _] = arch_str |> List.to_string() |> String.split("-")

        case arch do
          "x86_64" -> "#{osname}-x86_64"
          "aarch64" -> "#{osname}-aarch64"
          _ -> raise "sqlite-vss is not available for architecture: #{arch_str}"
        end
    end
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

    # https://erlef.github.io/security-wg/secure_coding_and_deployment_hardening/inets
    cacertfile = cacertfile() |> String.to_charlist()

    http_options =
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
      |> maybe_add_proxy_auth(scheme)

    options = [body_format: :binary]

    case :httpc.request(:get, {url, []}, http_options, options) do
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
    "https://github.com/asg017/sqlite-vss/releases/download/v$version/sqlite-vss-v$version-$file-$target.tar.gz"
  end

  defp get_url(base_url, file) do
    base_url
    |> String.replace("$version", configured_version())
    |> String.replace("$target", target())
    |> String.replace("$file", file)
  end
end
