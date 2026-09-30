// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/contents/memory.hpp"

#include "ttx/data/protocol/direct/provider.hpp"
#include "ttx/semantic/transport/flow.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Semantic::Transport;

auto Contents::Memory::supports(System::Uuid id) const -> Binding::Status {
  return id == Content::contract_id ||
                 id == Ttx::Semantic::Transport::Flow::direct.provider
             ? Binding::Status::Satisfied
             : Binding::Status::Unknown;
}

auto Contents::Memory::bind_interface(
    System::Uuid id,
    Ttx::Data::Form::Storage output) const -> Binding::Status {
  if (id == Content::contract_id) {
    const Content::Api api = {
      this,
      [](const void* self) -> U64 {
        return static_cast<const Contents::Memory*>(self)->bytes.get_size();
      },
      [](const void* self) -> ttx_semantic_query {
        return Ttx::Concept::Abstract::provide(
                   *static_cast<const Contents::Memory*>(self))
            .get_query();
      }};
    return Binding::provide<Content>(api, output);
  }

  if (id == Ttx::Semantic::Transport::Flow::direct.provider) {
    static const Ttx::Data::Protocol::Direct::Provider::Operations operations =
        {[](const void* self) -> const ttx_representation* {
           return &static_cast<const Contents::Memory*>(self)->representation;
         },
         [](const void* self) -> const void* {
           return static_cast<const Contents::Memory*>(self)->bytes.get_data();
         }};
    return Binding::provide<Ttx::Data::Protocol::Direct::Provider>(
        Ttx::Data::Protocol::Direct::Provider::Api(this, &operations), output);
  }

  return Binding::Status::Unknown;
}
