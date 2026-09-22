// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/storage.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Model::Type {

// A Type can admit storage without depending on an execution graph. The
// returned Representation describes this publication's selected realization. It
// borrows the provider, which can decline or defer the question without
// fabricating byte geometry. A different target policy can supply a different
// realization.
class Storage {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_TYPE_STORAGE_ID_HIGH,
    TETRODOTOXIN_MODEL_TYPE_STORAGE_ID_LOW,
  };
  using Api = tetrodotoxin_model_type_storage;

  explicit constexpr Storage(Api api) : api(api) {}

  auto get_representation() const -> Perimortem::Utility::Result<
      const Ttx::Data::Form::Representation&,
      Ttx::Semantic::Negotiation::Binding::Failure> {
    const ttx_representation* output = nullptr;
    const auto status = api.get_representation(api.source, &output);
    if (status == TTX_BINDING_SATISFIED) {
      return *output;
    }

    return static_cast<Ttx::Semantic::Negotiation::Binding::Failure>(status);
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Model::Type

TTX_DATA_RECORD(
    tetrodotoxin_model_type_storage,
    TTX_DATA_MEMBER(tetrodotoxin_model_type_storage, source),
    TTX_DATA_MEMBER(tetrodotoxin_model_type_storage, get_representation));
