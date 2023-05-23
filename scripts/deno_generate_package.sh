#!/bin/bash

set -euo pipefail

export PACKAGE_NAME="sqlite-vss"
export EXTENSION_NAME="vss0"
export VERSION=$(cat VERSION)

envsubst < bindings/deno/deno.json.tmpl > bindings/deno/deno.json
echo "✅ generated bindings/deno/deno.json"

envsubst < bindings/deno/README.md.tmpl > bindings/deno/README.md
echo "✅ generated bindings/deno/README.md"
