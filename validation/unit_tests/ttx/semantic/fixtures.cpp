// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
#include "validation/unit_tests/ttx/semantic/fixtures.hpp"
using namespace Validation::FlowTests;
Validation::Harness Validation::FlowTests::TtxFlow = {.name = "TTX::Flow"_view};

auto Validation::FlowTests::Reader::query() -> Query {
  using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Semantic::Transport;
  return Query(
      {this,
       [](const void* source, perimortem_uuid requested,
          ttx_storage result) -> ttx_binding_status {
         auto& reader =
             *const_cast<Validation::FlowTests::Reader*>(static_cast<const Validation::FlowTests::Reader*>(source));
         const auto schema = +[](const void* source) -> const Representation* {
           auto& reader =
               *const_cast<Validation::FlowTests::Reader*>(static_cast<const Validation::FlowTests::Reader*>(source));
           ++reader.descriptions;
           return &reader.schema;
         };
         static const Direct::View::Operations direct = {
           [](const void* source) -> const Representation* {
             auto& reader =
                 *const_cast<Validation::FlowTests::Reader*>(static_cast<const Validation::FlowTests::Reader*>(source));
             ++reader.descriptions;
             return reader.direct_schema ? reader.direct_schema
                                         : &reader.schema;
           }};
         static const Shared::View::Operations shared = {schema};
         static const Block::View::Operations block = {
           schema, [](const void*, ttx_storage storage) -> ttx_block_surface {
             return {storage.data, storage.representation->get_extent()};
           }};
         static const Fragment::View::Operations fragment = {schema};
         const Perimortem::System::Uuid id(requested);
         if (id == Direct::View::contract_id) {
           ++reader.binds[0];
           if (!(reader.provides & PROVIDES_DIRECT)) {
             return static_cast<ttx_binding_status>(reader.decline);
           }
           return static_cast<ttx_binding_status>(Binding::provide<Direct::View>(
               Direct::View::Api(source, &direct), Ttx::Data::Form::Storage(result)));
         } else if (id == Shared::View::contract_id) {
           ++reader.binds[1];
           if (!(reader.provides & PROVIDES_SHARED)) {
             return static_cast<ttx_binding_status>(reader.decline);
           }
           return static_cast<ttx_binding_status>(Binding::provide<Shared::View>(
               Shared::View::Api(source, &shared), Ttx::Data::Form::Storage(result)));
         } else if (id == Block::View::contract_id) {
           ++reader.binds[2];
           if (!(reader.provides & PROVIDES_BLOCK)) {
             return static_cast<ttx_binding_status>(reader.decline);
           }
           return static_cast<ttx_binding_status>(Binding::provide<Block::View>(
               Block::View::Api(source, &block), Ttx::Data::Form::Storage(result)));
         } else if (id == Fragment::View::contract_id) {
           ++reader.binds[3];
           if (!(reader.provides & PROVIDES_FRAGMENT)) {
             return static_cast<ttx_binding_status>(reader.decline);
           }
           return static_cast<ttx_binding_status>(Binding::provide<Fragment::View>(
               Fragment::View::Api(source, &fragment), Ttx::Data::Form::Storage(result)));
         } else {
           return TTX_BINDING_UNSUPPORTED;
         }
         return TTX_BINDING_SATISFIED;
       }});
}
