// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialect/library/dialect.hpp"

#include "tetrodotoxin/dialect/library/monograph.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;

auto Dialect::Library::Dialect::supports(System::Uuid id) const
    -> Binding::Status {
  return id == Source::Dialect::contract_id ? Binding::Status::Satisfied
                                            : Binding::Status::Unsupported;
}

auto Dialect::Library::Dialect::bind_interface(
    System::Uuid id,
    Ttx::Data::Form::Storage output) const -> Binding::Status {
  if (id != Source::Dialect::contract_id) {
    return Binding::Status::Unsupported;
  }
  const Source::Dialect::Api api = {
    this,
    [](const void* self, tetrodotoxin_source_cursor* cursor,
       ttx_abstract context, ttx_publication* result) -> ttx_binding_status {
      Source::Lexical::Cursor input(*cursor);
      auto publication = Monograph::interpret(
          input, Abstract(context),
          static_cast<const Dialect*>(self)->operation);
      cursor->index = input.get_index();
      return publication.visit(
          [&](Ttx::Semantic::Ownership::Publication& value)
              -> ttx_binding_status {
            *result = value.take();
            return TTX_BINDING_SATISFIED;
          },
          [](Binding::Failure failure) {
            return static_cast<ttx_binding_status>(failure);
          });
    }};
  return Binding::provide<Source::Dialect>(api, output);
}
