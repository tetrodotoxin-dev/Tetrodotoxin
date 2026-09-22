// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/model/type/conversions/identity.hpp"
#include "tetrodotoxin/model/type/policies/plain.hpp"
#include "tetrodotoxin/model/type/storage.hpp"

namespace Tetrodotoxin::Model::Type::Primitives {

// This provider gives a plain record the semantics of its admitted Data form.
// Its fields carry no extra restrictions beyond those described by that form.
// Padding and nested records are already compiled there, so consumers need no
// parallel tree of Type objects just to reproduce an observation. A language
// can expose richer field policies through a different provider or a layer
// that controls the corresponding conversion and assignment questions.
class Structure {
 public:
  explicit constexpr Structure(
      const Ttx::Data::Form::Representation& representation)
      : representation(representation),
        conversion(Policies::Plain::contract_id, representation) {}

  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }

  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Ttx::Semantic::Negotiation::Binding::Status;
    return id == Storage::contract_id || id == Policies::Plain::contract_id ||
                   id == Policies::Conversion::contract_id
               ? Status::Satisfied
               : Status::Unsupported;
  }

  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using namespace Ttx::Semantic::Negotiation;
    if (id == Policies::Conversion::contract_id) {
      return Binding::provide<Policies::Conversion>(
          conversion.get_interface().get_abi(), output);
    }

    if (id == Policies::Plain::contract_id) {
      return Binding::marker(output);
    }

    if (id == Storage::contract_id) {
      const Storage::Api api = {
        this,
        [](const void* self,
           const ttx_representation** result) -> ttx_binding_status {
          *result = &static_cast<const Structure*>(self)->representation;
          return TTX_BINDING_SATISFIED;
        }};
      return Binding::provide<Storage>(api, output);
    }

    return Binding::Status::Unsupported;
  }

 private:
  const Ttx::Data::Form::Representation& representation;
  Conversions::Identity conversion;
};

}  // namespace Tetrodotoxin::Model::Type::Primitives
