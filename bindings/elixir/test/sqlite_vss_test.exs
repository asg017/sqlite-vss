defmodule SqliteVssTest do
  use ExUnit.Case
  doctest SqliteVss

  test "loads the vss and vector path" do
    assert String.match?(SqliteVss.loadable_path_vss0(), ~r/vss0/)
    assert String.match?(SqliteVss.loadable_path_vector0(), ~r/vector0/)
  end
end
