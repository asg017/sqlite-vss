#!/bin/bash
set -euxo pipefail

#setopt sh_word_split
mkdir arm-homebrew && curl -L https://github.com/Homebrew/brew/tarball/master | tar xz --strip 1 -C arm-homebrew
response=$(./arm-homebrew/bin/brew fetch --force --bottle-tag=arm64_big_sur llvm | grep "Downloaded to")
parsed=($response)
echo $response
echo $parsed
./arm-homebrew/bin/brew install $parsed[3]