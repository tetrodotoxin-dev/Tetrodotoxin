// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Tetrodotoxin::Package {

// Storage retains its normalized diagnostic path beside the bytes for the
// opened acquisition transaction. Semantic graph owners copy any view they
// keep beyond that transaction.
class Content {
 public:
  constexpr Content(
      Perimortem::Core::View::Bytes diagnostic_path,
      Perimortem::Core::View::Bytes contents)
      : diagnostic_path(diagnostic_path), contents(contents) {}

  constexpr auto get_diagnostic_path() const -> Perimortem::Core::View::Bytes {
    return diagnostic_path;
  }

  constexpr auto get_contents() const -> Perimortem::Core::View::Bytes {
    return contents;
  }

 private:
  Perimortem::Core::View::Bytes diagnostic_path;
  Perimortem::Core::View::Bytes contents;
};

}  // namespace Tetrodotoxin::Package
