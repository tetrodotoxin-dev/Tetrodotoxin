// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/model/type/policies/flag.h"
#include "ttx/semantic/negotiation/binding.hpp"

namespace Tetrodotoxin::Model::Type::Policies {

// A type may expose a truth interpretation independently of its numeric and
// storage policies. The input is an already materialized observation so this
// operation introduces neither a transport preference nor a second lifetime.
class Flag {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TETRODOTOXIN_MODEL_TYPE_FLAG_ID_HIGH,
    TETRODOTOXIN_MODEL_TYPE_FLAG_ID_LOW,
  };
  using Api = tetrodotoxin_model_type_flag;
  explicit constexpr Flag(Api api) : api(api) {}

  auto get_truth(Ttx::Data::Form::Storage value) const
      -> Perimortem::Utility::Result<Bool, Ttx::Data::Status> {
    U8 result = 0;
    const auto status = api.get_truth(api.source, value.get_abi(), &result);
    if (status != TTX_DATA_SUCCESS) {
      return static_cast<Ttx::Data::Status>(status);
    }

    return Bool(result != 0);
  }

 private:
  Api api;
};

}  // namespace Tetrodotoxin::Model::Type::Policies

TTX_DATA_RECORD(
    tetrodotoxin_model_type_flag,
    TTX_DATA_MEMBER(tetrodotoxin_model_type_flag, source),
    TTX_DATA_MEMBER(tetrodotoxin_model_type_flag, get_truth));
