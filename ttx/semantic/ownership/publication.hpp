// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/semantic/ownership/publication.h"
#include "ttx/semantic/negotiation/query.hpp"

namespace Ttx::Semantic::Ownership {

// Publication owns the state behind a borrowed Query. Moving it transfers the
// release obligation, so source discovery and runtime execution can have their
// own lifetimes without adding retain calls to every binding.
class Publication {
 public:
  explicit Publication(ttx_publication value) : value(value) {}
  Publication(const Publication&) = delete;
  auto operator=(const Publication&) -> Publication& = delete;
  Publication(Publication&& other) : value(other.take()) {}
  ~Publication() { close(); }

  auto get_query() const -> Negotiation::Query { return Negotiation::Query(value.query); }
  auto take() -> ttx_publication {
    auto result = value;
    value = {};
    return result;
  }
  auto close() -> void {
    if (value.release) {
      auto previous = take();
      previous.release(previous.query.source);
    }
  }

 private:
  ttx_publication value;
};

}  // namespace Ttx::Semantic::Ownership

TTX_DATA_RECORD(
    ttx_publication,
    TTX_DATA_MEMBER(ttx_publication, query),
    TTX_DATA_MEMBER(ttx_publication, release));
