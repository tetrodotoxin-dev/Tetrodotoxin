// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/data/status.hpp"
#include "ttx/semantic/ownership/factory.h"
#include "ttx/semantic/ownership/publication.hpp"

namespace Ttx::Semantic::Ownership {

// Factory is the runtime half of construction. It contains no Abstract edge,
// allowing a terminal to discard the graph that selected this implementation.
class Factory {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_FACTORY_ID_HIGH,
    TTX_FACTORY_ID_LOW,
  };
  using Api = ttx_factory;
  using Operations = ttx_factory_operations;

  explicit constexpr Factory(Api api) : api(api) {}

  auto create() const
      -> Perimortem::Utility::Result<Publication, Data::Status> {
    ttx_publication output = {};
    const auto status = api.operations->create(api.source, &output);
    if (status != TTX_DATA_SUCCESS) {
      return static_cast<Data::Status>(status);
    }

    return Publication(output);
  }

 private:
  Api api;
};

}  // namespace Ttx::Semantic::Ownership

TTX_DATA_RECORD(
    ttx_factory_operations,
    TTX_DATA_MEMBER(ttx_factory_operations, create));

TTX_DATA_RECORD(
    ttx_factory,
    TTX_DATA_MEMBER(ttx_factory, source),
    TTX_DATA_MEMBER(ttx_factory, operations));
