#!/bin/bash

set -euo pipefail

export PACKAGE_NAME_BASE="sqlite-vss"
export EXTENSION_NAME="vss0"
export VERSION=$(cat VERSION)

generate () {
  export PLATFORM_OS=$1
  export PLATFORM_ARCH=$2
  export PACKAGE_NAME=$PACKAGE_NAME_BASE-$PLATFORM_OS-$PLATFORM_ARCH

  if [ "$PLATFORM_OS" == "windows" ]; then
    export EXTENSION_SUFFIX="dll"
  elif [ "$PLATFORM_OS" == "darwin" ]; then
    export EXTENSION_SUFFIX="dylib"
  else
    export EXTENSION_SUFFIX="so"
  fi


  mkdir -p bindings/node/$PACKAGE_NAME/lib

  envsubst < bindings/node/platform-package.package.json.tmpl > bindings/node/$PACKAGE_NAME/package.json
  envsubst < bindings/node/platform-package.README.md.tmpl > bindings/node/$PACKAGE_NAME/README.md

  touch bindings/node/$PACKAGE_NAME/lib/.gitkeep

  echo "✅ generated bindings/node/$PACKAGE_NAME"
}

envsubst < bindings/node/$PACKAGE_NAME_BASE/package.json.tmpl > bindings/node/$PACKAGE_NAME_BASE/package.json
echo "✅ generated bindings/node/$PACKAGE_NAME_BASE"

generate darwin x64
generate darwin arm64
generate linux x64
