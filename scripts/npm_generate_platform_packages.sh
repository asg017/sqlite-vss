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

  
  mkdir -p npm/$PACKAGE_NAME/lib

  envsubst < npm/platform-package.package.json.tmpl > npm/$PACKAGE_NAME/package.json
  envsubst < npm/platform-package.README.md.tmpl > npm/$PACKAGE_NAME/README.md
  
  touch npm/$PACKAGE_NAME/lib/.gitkeep
  
  echo "✅ generated npm/$PACKAGE_NAME"
}

envsubst < npm/$PACKAGE_NAME_BASE/package.json.tmpl > npm/$PACKAGE_NAME_BASE/package.json
echo "✅ generated npm/$PACKAGE_NAME_BASE"

generate darwin x64 
generate darwin arm64 
generate linux x64 