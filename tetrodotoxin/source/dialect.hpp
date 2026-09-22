// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/dialect.h"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "ttx/semantic/ownership/publication.hpp"

namespace Tetrodotoxin::Source {

// Dialect executes through the supplied Cursor and publishes its Monograph.
// This facade preserves the C lifetime boundary, allowing a foreign dialect to
// own its graph without adopting the caller's allocation or diagnostic system.
class Dialect {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_SOURCE_DIALECT_ID_HIGH, TETRODOTOXIN_SOURCE_DIALECT_ID_LOW};
  using Api = tetrodotoxin_source_dialect;
  explicit constexpr Dialect(Api api) : api(api) {}

  auto interpret(Lexical::Cursor& cursor, Ttx::Concept::Abstract context) const
      -> Perimortem::Utility::Result<
          Ttx::Semantic::Ownership::Publication,
          Ttx::Semantic::Negotiation::Binding::Failure> {
    ttx_publication output = {};
    auto position = cursor.get_abi();
    const auto status =
        api.interpret(api.source, &position, context.get_abi(), &output);

    // Failure may follow valid consumption. Publish that progress as well as
    // successful progress, invalidating the caller's cached observation.
    cursor.set_index(position.index);
    if (status != TTX_BINDING_SATISFIED) {
      return static_cast<Ttx::Semantic::Negotiation::Binding::Failure>(status);
    }

    return Ttx::Semantic::Ownership::Publication(output);
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Source

TTX_DATA_RECORD(
    tetrodotoxin_source_dialect,
    TTX_DATA_MEMBER(tetrodotoxin_source_dialect, source),
    TTX_DATA_MEMBER(tetrodotoxin_source_dialect, interpret));
