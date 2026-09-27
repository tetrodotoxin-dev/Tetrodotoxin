#!/usr/bin/env bash

set -euo pipefail

repository="$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)"
cd "$repository"

format_ttx() {
  if (($# == 0)); then
    return
  fi

  bazel build //source/puffer:puffer >/dev/null
  .bin/bin/source/puffer/puffer -format "$@"
}

if (($# > 0)); then
  cpp_files=()
  ttx_files=()
  for path in "$@"; do
    case "$path" in
      *.cpp | *.hpp) cpp_files+=("$path") ;;
      *.ttx) ttx_files+=("$path") ;;
    esac
  done

  if ((${#cpp_files[@]} > 0)); then
    clang-format -i -- "${cpp_files[@]}"
  fi
  format_ttx "${ttx_files[@]}"
  exit 0
fi

# ripgrep honors the repository ignore rules, keeping generated Bazel trees and
# external dependencies out of the formatter input.
rg --files --null -g '*.cpp' -g '*.hpp' |
  xargs --null --no-run-if-empty clang-format -i

mapfile -d '' ttx_files < <(rg --files --null -g '*.ttx')
format_ttx "${ttx_files[@]}"
