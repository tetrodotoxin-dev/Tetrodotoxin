// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/modules/import.h"
#include "ttx/concept/modules/module.hpp"

namespace Ttx::Concept::Modules {

// Import lends the caller's acquisition policy to a provider. The resulting
// Module retains its acquisition implementation, so a failed replacement leaves
// an existing callable's independently retained implementation intact.
class Import {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_IMPORT_ID_HIGH,
    TTX_IMPORT_ID_LOW,
  };
  using Api = ttx_import;
  using Operations = ttx_import_operations;

  explicit constexpr Import(Api api) : api(api) {}

  auto open(Perimortem::Core::View::Bytes name) const
      -> Perimortem::Utility::Result<Module, Data::Status> {
    ttx_module output = {};
    const auto status = api.operations->open(
        api.source, {name.get_data(), name.get_size()}, &output);
    if (status != TTX_DATA_SUCCESS) {
      return static_cast<Data::Status>(status);
    }

    return Module(output);
  }

 private:
  Api api;
};

}  // namespace Ttx::Concept::Modules

TTX_DATA_RECORD(
    ttx_import_operations,
    TTX_DATA_MEMBER(ttx_import_operations, open));

TTX_DATA_RECORD(
    ttx_import,
    TTX_DATA_MEMBER(ttx_import, source),
    TTX_DATA_MEMBER(ttx_import, operations));
