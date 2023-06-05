#!/bin/bash

# ./scripts/elixir_generate_checksum.sh "$(cat ~/tmp/checksums.txt)"

CHECKSUMS_TXT="$1"


generate () {
  export PLATFORM=$1
  export TARGET_ENV_NAME=$2
  export TARGET_ENV_SHA=$3
  result="$(echo "$CHECKSUMS_TXT" | grep "loadable-$PLATFORM")"

  NAME=$(echo "$result" | sed -E 's/.* (.*)/\1/')
  CHECKSUM=$(echo "$result" | sed -E 's/([^ ]*) .*/\1/')

  export "$TARGET_ENV_NAME"="$NAME"
  export "$TARGET_ENV_SHA"="$CHECKSUM"
}


generate linux-x86_64 LOADABLE_ASSET_LINUX_X86_64_NAME LOADABLE_ASSET_LINUX_X86_64_SHA256
generate macos-x86_64 LOADABLE_ASSET_MACOS_X86_64_NAME LOADABLE_ASSET_MACOS_X86_64_SHA256
generate macos-aarch64 LOADABLE_ASSET_MACOS_AARCH64_NAME LOADABLE_ASSET_MACOS_AARCH64_SHA256


envsubst < bindings/elixir/sqlite-vss-checksum.exs.tmpl > bindings/elixir/sqlite-vss-checksum.exs
echo "✅ generated bindings/elixir/sqlite-vss-checksum.exs"
