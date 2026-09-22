// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/contracts/documentation.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Source::Contracts {

// A declaration can return documentation supplied by another language without
// constructing a native Documentation subclass. This record borrows its line
// operations directly. The enclosing declaration publication keeps the prose
// and its provider alive while a consumer reads it.
class Documentation {
 public:
  using Api = tetrodotoxin_source_documentation;

  explicit constexpr Documentation(Api api) : api(api) {}

  constexpr auto get_abi() const -> Api { return api; }

  auto get_line(Count index) const -> Perimortem::Core::View::Bytes {
    const auto line = api.operations->get_line(api.source, index);
    return Perimortem::Core::View::Bytes(line.data, line.size);
  }

  auto line_count() const -> Count {
    return api.operations->line_count(api.source);
  }

  auto is_empty() const -> Bool { return line_count() == 0; }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Source::Contracts

TTX_DATA_RECORD(
    tetrodotoxin_source_documentation_ops,
    TTX_DATA_MEMBER(tetrodotoxin_source_documentation_ops, line_count),
    TTX_DATA_MEMBER(tetrodotoxin_source_documentation_ops, get_line));
TTX_DATA_RECORD(
    tetrodotoxin_source_documentation,
    TTX_DATA_MEMBER(tetrodotoxin_source_documentation, source),
    TTX_DATA_MEMBER(tetrodotoxin_source_documentation, operations));
