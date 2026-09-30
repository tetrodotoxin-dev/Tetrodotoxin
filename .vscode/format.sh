#!/usr/bin/env bash
set -euo pipefail
if (($# == 0)); then
  echo 'Pass the exact C or C++ files to format.' >&2
  exit 1
fi
clang-format -i -- "$@"
