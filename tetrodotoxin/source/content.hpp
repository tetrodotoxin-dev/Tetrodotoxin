// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/content.h"
#include "ttx/semantic/negotiation/query.hpp"

namespace Tetrodotoxin::Source {

// Content separates source observation from interpretation. Binding it gives
// the caller the immutable byte contract, while get_data selects access to
// those bytes through ordinary Semantic transport negotiation. No tokenizer
// or filesystem object needs to survive in the consumer's interface.
class Content {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_SOURCE_CONTENT_ID_HIGH, TETRODOTOXIN_SOURCE_CONTENT_ID_LOW};
  using Api = tetrodotoxin_source_content;

  explicit constexpr Content(Api api) : api(api) {}

  auto get_size() const -> Count { return api.get_size(api.source); }
  auto get_data() const -> Ttx::Semantic::Negotiation::Query {
    return Ttx::Semantic::Negotiation::Query(api.get_data(api.source));
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Source

TTX_DATA_RECORD(
    tetrodotoxin_source_content,
    TTX_DATA_MEMBER(tetrodotoxin_source_content, source),
    TTX_DATA_MEMBER(tetrodotoxin_source_content, get_size),
    TTX_DATA_MEMBER(tetrodotoxin_source_content, get_data));
