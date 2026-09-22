// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/conversions/identity.hpp"
#include "tetrodotoxin/model/type/policies/boolean.hpp"
#include "tetrodotoxin/model/type/policies/flag.hpp"
#include "tetrodotoxin/model/type/policies/real.hpp"
#include "tetrodotoxin/model/type/policies/signed.hpp"
#include "tetrodotoxin/model/type/policies/unsigned.hpp"
#include "tetrodotoxin/model/type/storage.hpp"

namespace Tetrodotoxin::Model::Type::Primitives {

// A native scalar combines a numeric interpretation with one selected storage
// representation. The template parameter describes this provider's concrete
// choice only. Consumers ask its contracts and can substitute a foreign owner
// without knowing the C++ specialization that published these answers.
template <typename Value>
class Scalar {
 public:
  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }

  static auto get_representation() -> const Ttx::Data::Form::Representation& {
    return Ttx::Data::Form::Compiled<
        Ttx::Data::Form::Native<Value>::reference>::get_representation();
  }

  static constexpr auto get_policy() -> Perimortem::System::Uuid {
    if constexpr (__is_same(Value, bool)) {
      return Policies::Boolean::contract_id;
    } else if constexpr (__is_floating_point(Value)) {
      return Policies::Real::contract_id;
    } else if constexpr (Value(-1) < Value(0)) {
      return Policies::Signed::contract_id;
    } else {
      return Policies::Unsigned::contract_id;
    }
  }

  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Ttx::Semantic::Negotiation::Binding::Status;
    return id == get_policy() || id == Storage::contract_id ||
                   id == Policies::Conversion::contract_id ||
                   (__is_same(Value, bool) && id == Policies::Flag::contract_id)
               ? Status::Satisfied
               : Status::Unsupported;
  }

  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using namespace Ttx::Semantic::Negotiation;
    if (id == Policies::Conversion::contract_id) {
      static const Conversions::Identity conversion(
          get_policy(), get_representation());
      return Binding::provide<Policies::Conversion>(
          conversion.get_interface().get_abi(), output);
    }

    if constexpr (__is_same(Value, bool)) {
      if (id == Policies::Flag::contract_id) {
        const Policies::Flag::Api api = {
          this,
          [](const void*, ttx_storage input, U8* result) -> ttx_data_status {
            if (!get_representation().compatible(*input.representation)) {
              return TTX_DATA_INCOMPATIBLE;
            }

            *result = *input.data != 0;
            return TTX_DATA_SUCCESS;
          }};
        return Binding::provide<Policies::Flag>(api, output);
      }
    }

    if (id == get_policy()) {
      return Binding::marker(output);
    }

    if (id == Storage::contract_id) {
      const Storage::Api api = {
        this,
        [](const void*,
           const ttx_representation** result) -> ttx_binding_status {
          *result = &get_representation();
          return TTX_BINDING_SATISFIED;
        }};
      return Binding::provide<Storage>(api, output);
    }

    return Binding::Status::Unsupported;
  }
};

}  // namespace Tetrodotoxin::Model::Type::Primitives
