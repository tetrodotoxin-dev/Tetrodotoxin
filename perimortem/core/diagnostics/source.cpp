// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/diagnostics/source.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;

auto Diagnostics::Source::is_set() const -> Bool {
  return impl != nullptr || !file.is_empty();
}

auto Diagnostics::Source::get_line() const -> Count {
  if (!file.is_empty()) {
    return line;
  }

  if (impl == nullptr) {
    return 0;
  }

  return Count(impl->_M_line);
}

auto Diagnostics::Source::get_column() const -> Count {
  if (!file.is_empty()) {
    return column;
  }

  if (impl == nullptr) {
    return 0;
  }

  return Count(impl->_M_column);
}

auto Diagnostics::Source::get_file() const -> Core::View::Bytes {
  if (!file.is_empty()) {
    return file;
  }

  if (impl == nullptr) {
    return Core::View::Bytes();
  }

  return NullTerminated::to_view(impl->_M_file_name);
}

auto Diagnostics::Source::get_function() const -> Core::View::Bytes {
  if (!file.is_empty()) {
    return function;
  }

  if (impl == nullptr) {
    return Core::View::Bytes();
  }

  return NullTerminated::to_view(impl->_M_function_name);
}
