#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)

cd "$repo_root"

# Allow for configuration to be passed in for Bazel
configuration=debug
if [[ ${1:-} == --release ]]; then
    configuration=release
elif [[ $# -gt 0 && $1 != --debug ]]; then
    echo "Usage: puffer/package.sh [--debug|--release]" >&2
    exit 2
fi

# Set the SDK root and make sure we have a `bin` and `standard` packages folder.
sdk_root="$repo_root/.bin/puffer-sdk"
mkdir -p "$sdk_root/bin" "$sdk_root/standard"

# Build Puffer using Bazel since we don't have a C++ puffer toolchain yet. 
bazel build --config="$configuration" //:sdk

# Replace the executable inode so an editor can keep using its loaded compiler.
staging=$(mktemp -d "$sdk_root/.install.XXXXXX")
trap 'rm -rf -- "$staging"' EXIT
archive=$(bazel cquery --config="$configuration" //:sdk --output=files)
tar -xf "$archive" -C "$staging"
for file in "$staging"/*; do
    mv -f -- "$file" "$sdk_root/bin/"
done
for package in Perimortem.Memory Perimortem.Math Perimortem.System Perimortem.Graphics; do
    mkdir -p "$sdk_root/standard/$package"
    ln -sfn "$repo_root/packages/ttx/$package" "$sdk_root/standard/$package/1.0"
done

echo "Puffer SDK prepared in $sdk_root"
