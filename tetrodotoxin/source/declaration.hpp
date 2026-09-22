// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/system/uuid.hpp"

#include "tetrodotoxin/source/anchor.hpp"
#include "tetrodotoxin/source/declaration.h"
#include "ttx/semantic/negotiation/binding.hpp"

namespace Tetrodotoxin::Source {

// Declaration borrows a provider's source observation. Its API record is the
// complete negotiated value, so foreign providers need neither a C++ base
// class nor a second wrapper object. Evaluation belongs to the language query
// being answered. Reading provenance does not require evaluating that value.
class Declaration {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_SOURCE_DECLARATION_ID_HIGH,
    TETRODOTOXIN_SOURCE_DECLARATION_ID_LOW,
  };

  using Api = tetrodotoxin_source_declaration;

  explicit constexpr Declaration(Api api) : api(api) {}

  auto get_anchor() const -> Perimortem::Core::Option<Anchor> {
    auto anchor = Anchor(Ttx::Concept::Abstract(ttx_none()), Range());
    if (!api.get_anchor(api.source, &anchor)) {
      return {};
    }

    return anchor;
  }

  template <typename Provider>
  static auto provide(
      const Provider& provider,
      Ttx::Data::Form::Storage destination)
      -> Ttx::Semantic::Negotiation::Binding::Status {
    const Api api = {
      &provider, [](const void* source, Anchor* output) -> U8 {
        return static_cast<const Provider*>(source)->get_anchor().visit(
            []() -> U8 { return 0; },
            [&](Anchor anchor) -> U8 {
              *output = anchor;
              return 1;
            });
      }};
    return Ttx::Semantic::Negotiation::Binding::provide<Declaration>(
        api, destination);
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Source

TTX_DATA_RECORD(
    tetrodotoxin_source_declaration,
    TTX_DATA_MEMBER(tetrodotoxin_source_declaration, source),
    TTX_DATA_MEMBER(tetrodotoxin_source_declaration, get_anchor));
