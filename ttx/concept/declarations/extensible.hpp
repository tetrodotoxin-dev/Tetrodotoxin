// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/declarations/extensible.h"
#include "ttx/data/status.hpp"
#include "ttx/semantic/ownership/publication.hpp"

namespace Ttx::Concept::Declarations {

// Extensible connects a declaration to its executable construction capability.
// The terminal retains the emitted publication so it can release this borrowed
// declaration graph before constructing runtime instances.
class Extensible {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_EXTENSIBLE_ID_HIGH,
    TTX_EXTENSIBLE_ID_LOW,
  };
  using Api = ttx_extensible;
  using Operations = ttx_extensible_operations;
  explicit constexpr Extensible(Api api) : api(api) {}

  auto emit_factory() const -> Perimortem::Utility::
      Result<Semantic::Ownership::Publication, Data::Status> {
    ttx_publication output = {};
    const auto status = api.operations->emit_factory(api.source, &output);
    if (status != TTX_DATA_SUCCESS) {
      return static_cast<Data::Status>(status);
    }

    return Semantic::Ownership::Publication(output);
  }

 private:
  Api api;
};

}  // namespace Ttx::Concept::Declarations

TTX_DATA_RECORD(
    ttx_extensible_operations,
    TTX_DATA_MEMBER(ttx_extensible_operations, emit_factory));

TTX_DATA_RECORD(
    ttx_extensible,
    TTX_DATA_MEMBER(ttx_extensible, source),
    TTX_DATA_MEMBER(ttx_extensible, operations));
