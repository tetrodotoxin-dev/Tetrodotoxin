// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// Should only be included in cpp files
// #pragma once

#include "perimortem/core/view/bytes.hpp"

// Compiler extension ABI

namespace std {

class source_location {
 public:
  struct __impl {
    // Null terminated cstring
    const char* _M_file_name;
    // Null terminated cstring
    const char* _M_function_name;
    unsigned _M_line;
    unsigned _M_column;
  };
};

}  // namespace std

namespace Perimortem::Core::Diagnostics {

// Thin Perimortem shim over the compiler's source location ABI.
// Source stores only a pointer to the static __impl struct baked into the
// binary and uses value semantics so a Source copy is always a single pointer
// copy.
//
// Other runtimes can provide source information using the same fields, which
// keeps diagnostics useful across language boundaries.
//
// Accessors are evaluated lazily since evaluating source most likely means we
// are already in a diagnostics slow path.
struct Source {
 public:
  // Captures the call site via the standard function default parameter pattern.
  // The __impl pointer points to read only data in the binary.
  static consteval auto current(
      const std::source_location::__impl* impl = __builtin_source_location())
      -> Source {
    return Source(impl);
  }

  constexpr Source() = default;
  constexpr Source(const std::source_location::__impl* impl) : impl(impl) {}
  constexpr Source(
      Core::View::Bytes file,
      Count line,
      Count column,
      Core::View::Bytes function = Core::View::Bytes())
      : file(file), function(function), line(line), column(column) {}

  auto is_set() const -> Bool;
  auto get_line() const -> Count;
  auto get_column() const -> Count;
  auto get_file() const -> Core::View::Bytes;
  auto get_function() const -> Core::View::Bytes;

 private:
  const std::source_location::__impl* impl = nullptr;
  Core::View::Bytes file;
  Core::View::Bytes function;
  Count line = 0;
  Count column = 0;
};

}  // namespace Perimortem::Core::Diagnostics
