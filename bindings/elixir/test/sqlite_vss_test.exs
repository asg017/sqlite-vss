defmodule SqliteVssTest do
  use ExUnit.Case
  doctest SqliteVss

  import ExUnit.CaptureLog

  describe "current_target/1" do
    test "aarch64 in an Apple machine with Darwin-based OS" do
      target_system = %{arch: "aarch64", vendor: "apple", os: "darwin20.3.0"}

      config = %{
        target_system: target_system,
        os_type: {:unix, :darwin}
      }

      assert {:ok, "macos-aarch64"} = SqliteVss.current_target(config)
    end

    test "x86_64 in an Apple machine with Darwin-based OS" do
      target_system = %{arch: "x86_64", vendor: "apple", os: "darwin20.3.0"}

      config = %{
        target_system: target_system,
        os_type: {:unix, :darwin}
      }

      assert {:ok, "macos-x86_64"} = SqliteVss.current_target(config)
    end

    test "x86_64 in a PC running RedHat Linux" do
      target_system = %{arch: "x86_64", vendor: "redhat", os: "linux", abi: "gnu"}

      config = %{
        target_system: target_system,
        os_type: {:unix, :linux}
      }

      assert {:ok, "linux-x86_64"} = SqliteVss.current_target(config)
    end

    test "aarch64 in a PC running Linux" do
      target_system = %{arch: "aarch64", vendor: "pc", os: "linux", abi: "gnu"}

      config = %{
        target_system: target_system,
        os_type: {:unix, :linux}
      }

      assert {:ok, "linux-aarch64"} = SqliteVss.current_target(config)
    end

    test "target not available" do
      config = %{
        target_system: %{arch: "i686", vendor: "unknown", os: "linux", abi: "gnu"},
        nif_version: "2.14",
        os_type: {:unix, :linux}
      }

      error_message =
        """
        precompiled artifact is not available for this target: \"i686-unknown-linux-gnu\".
        The available targets are:
         - linux-x86_64
         - macos-aarch64
         - macos-x86_64
        """
        |> String.trim()

      assert {:error, ^error_message} = SqliteVss.current_target(config)
    end
  end

  @tag :tmp_dir
  test "check_integrity_from_map/3", %{tmp_dir: tmp_dir} do
    content = """
    Roses are red
    Violets are blue
    """

    file_path = Path.join(tmp_dir, "poem.txt")
    :ok = File.write(file_path, content)

    # the checksum is calculated with `:crypto.hash(:sha256, content) |> Base.encode16(case: :lower)`
    checksum_map = %{
      "poem.txt" => "sha256:fe16da553f29a704ad4c78624bc9354b8e4df6e4de8edb5b0f8d9f9090501911"
    }

    assert :ok = SqliteVss.check_integrity_from_map(checksum_map, file_path)
    SqliteVss.check_integrity_from_map(checksum_map, "idontexist")

    assert {:error,
            "the precompiled artifact does not exist in the checksum file. Please consider running: `mix sqlite_vss.install` to generate the checksum file."} =
             SqliteVss.check_integrity_from_map(checksum_map, "idontexist")

    :ok = File.write(file_path, "let's change the content of the file")

    assert {:error, "the integrity check failed because the checksum of files does not match"} =
             SqliteVss.check_integrity_from_map(checksum_map, file_path)

    wrong_file_path = Path.join(tmp_dir, "i-dont-exist/poem.txt")

    assert {:error, message} = SqliteVss.check_integrity_from_map(checksum_map, wrong_file_path)

    assert message =~ "cannot read the file for checksum comparison: "
    assert message =~ wrong_file_path
    assert message =~ "Reason: :enoent"
  end

  describe "install/0" do
    setup do
      root_path = File.cwd!()
      extension_fixtures_dir = Path.join(root_path, "test/fixtures")
      checksum_sample_file = Path.join(extension_fixtures_dir, "checksum-sqlite-vss.exs")
      checksum_sample = File.read!(checksum_sample_file)

      {:ok, extension_fixtures_dir: extension_fixtures_dir, checksum_sample: checksum_sample}
    end

    @tag :tmp_dir
    test "a project using precompiled extensions from cache", %{
      tmp_dir: tmp_dir,
      checksum_sample: checksum_sample,
      extension_fixtures_dir: extension_fixtures_dir
    } do
      in_tmp(tmp_dir, fn ->
        File.write!("checksum-sqlite-vss.exs", checksum_sample)

        result =
          capture_log(fn ->
            assert :ok = SqliteVss.install()
          end)

        refute result =~ "Downloading"
        refute result =~ "http://localhost"
        assert result =~ "from cache"
      end)
    end

    @tag :tmp_dir
    test "a project downloading precompiled extensions", %{
      tmp_dir: tmp_dir,
      checksum_sample: checksum_sample,
      extension_fixtures_dir: extension_fixtures_dir
    } do
      bypass = Bypass.open()

      in_tmp(tmp_dir, fn ->
        File.write!("checksum-sqlite-vss.exs", checksum_sample)

        Bypass.expect_once(bypass, fn conn ->
          file_name = List.last(conn.path_info)

          file =
            File.read!(Path.join([extension_fixtures_dir, "precompiled_extensions", file_name]))

          Plug.Conn.resp(conn, 200, file)
        end)

        result =
          capture_log(fn ->
            assert :ok = SqliteVss.install()
          end)

        assert result =~ "Downloading"
        assert result =~ "http://localhost:#{bypass.port}/download"
        assert result =~ "artifact cached at"
      end)
    end

    @tag :tmp_dir
    test "a project downloading precompiled extensions without the checksum file", %{
      tmp_dir: tmp_dir,
      extension_fixtures_dir: extension_fixtures_dir
    } do
      bypass = Bypass.open()

      in_tmp(tmp_dir, fn ->
        Bypass.expect_once(bypass, fn conn ->
          file_name = List.last(conn.path_info)
          file = File.read!(Path.join([extension_fixtures_dir, "precompiled_nifs", file_name]))

          Plug.Conn.resp(conn, 200, file)
        end)

        capture_log(fn ->
          assert {:error, error} = SqliteVss.install()

          assert error =~
                   "the precompiled artifact file does not exist in the checksum file. " <>
                     "Please consider run: `mix sqlite_vss.download` " <>
                     "to generate the checksum file."
        end)
      end)
    end
  end

  test "loads the vss and vector path" do
    assert String.match?(SqliteVss.loadable_path_vss0(), ~r/vss0/)
    assert String.match?(SqliteVss.loadable_path_vector0(), ~r/vector0/)
  end

  def in_tmp(tmp_path, function) do
    path = Path.join([tmp_path, random_string(10)])

    try do
      File.rm_rf(path)
      File.mkdir_p!(path)
      File.cd!(path, function)
    after
      File.rm_rf(path)
      :ok
    end
  end

  defp random_string(len) do
    len
    |> :crypto.strong_rand_bytes()
    |> Base.encode64()
    |> binary_part(0, len)
  end
end
