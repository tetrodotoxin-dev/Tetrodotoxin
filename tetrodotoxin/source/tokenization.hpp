// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/tokenization.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/semantic/ownership/publication.hpp"

namespace Tetrodotoxin::Source {

// Tokenization returns the selected provider's publication, allowing a C or
// C++ lexer to own its storage without making its allocator part of the API.
// The consumer binds the returned token contract separately. This keeps a
// language's lexical vocabulary out of the common Source observation contract.
class Tokenization {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_SOURCE_TOKENIZATION_ID_HIGH,
    TETRODOTOXIN_SOURCE_TOKENIZATION_ID_LOW};
  using Api = tetrodotoxin_source_tokenization;

  explicit constexpr Tokenization(Api api) : api(api) {}

  auto tokenize(Ttx::Concept::Abstract observation) const
      -> Perimortem::Utility::Result<
          Ttx::Semantic::Ownership::Publication,
          Ttx::Semantic::Negotiation::Binding::Failure> {
    ttx_publication output = {};
    const auto status =
        api.tokenize(api.source, observation.get_abi(), &output);
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
    tetrodotoxin_source_tokenization,
    TTX_DATA_MEMBER(tetrodotoxin_source_tokenization, source),
    TTX_DATA_MEMBER(tetrodotoxin_source_tokenization, tokenize));
