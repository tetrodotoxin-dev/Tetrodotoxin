// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/model/type/conversions/identity.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;
using namespace Tetrodotoxin::Model::Type;

auto Conversions::Identity::convert(Abstract value) const
    -> Utility::Result<Abstract, Binding::Failure> {
  using Result = Utility::Result<Abstract, Binding::Failure>;
  return value.bind<Domain>().visit(
      [&](Domain domain) -> Result {
        return domain.get_domain().visit(
            [&](Abstract type) -> Result {
              const auto status = type.supports(policy);
              if (status != Binding::Status::Satisfied) {
                return static_cast<Binding::Failure>(status);
              }

              return type.bind<Storage>().visit(
                  [&](Storage storage) -> Result {
                    return storage.get_representation().visit(
                        [&](const auto& actual) -> Result {
                          if (!representation.compatible(actual)) {
                            return Binding::Failure::Rejected;
                          }

                          return value;
                        },
                        [](Binding::Failure failure) -> Result {
                          return failure;
                        });
                  },
                  [](Binding::Failure failure) -> Result { return failure; });
            },
            [](Binding::Failure failure) -> Result { return failure; });
      },
      [](Binding::Failure failure) -> Result { return failure; });
}

auto Conversions::Identity::get_interface() const -> Policies::Conversion {
  return Policies::Conversion(
      {this,
       [](const void* source, ttx_abstract value,
          ttx_abstract* output) -> ttx_binding_status {
         return static_cast<const Identity*>(source)
             ->convert(Abstract(value))
             .visit(
                 [&](Abstract answer) -> ttx_binding_status {
                   *output = answer.get_abi();
                   return TTX_BINDING_SATISFIED;
                 },
                 [](Binding::Failure failure) {
                   return static_cast<ttx_binding_status>(failure);
                 });
       }});
}
