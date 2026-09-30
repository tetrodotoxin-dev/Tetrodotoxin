// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialects/source/capabilities/diagnostics.hpp"
#include "tetrodotoxin/dialects/source/capabilities/parse.h"
#include "tetrodotoxin/dialects/source/cursor.hpp"

namespace Tetrodotoxin::Dialects::Source::Capabilities {

// Parse's facade passes the caller's actual position through the C boundary.
// The owner chooses lexical admission and result storage. The native adapter
// negotiates Diagnostics once and exposes no native collector to that owner.
class Parse : public Ttx::Concept::Abstract {
 public:
  using Api = tetrodotoxin_source_parse;
  using Operations = tetrodotoxin_source_parse_ops;
  static constexpr auto contract_id = Perimortem::System::Uuid(
      TETRODOTOXIN_SOURCE_PARSE_ID_HIGH,
      TETRODOTOXIN_SOURCE_PARSE_ID_LOW);
  static auto accept(Api value) -> Bool {
    return value.operations && value.operations->parse &&
           Abstract::accept({value.source, &value.operations->abstract});
  }
  explicit constexpr Parse(Api value)
      : Abstract(value.source, value.operations->abstract) {}
  constexpr auto get_abi() const -> Api {
    auto value = Abstract::get_abi();
    return {
      value.source, reinterpret_cast<const Operations*>(value.operations)};
  }
  template <typename Receiver>
  auto parse(Cursor& cursor, Abstract diagnostics, Receiver& receiver) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    auto value = get_abi();
    return static_cast<Ttx::Semantic::Negotiation::Binding::Status>(
        value.operations->parse(
            value.source, &cursor.get_abi(), diagnostics.get_abi(), &receiver,
            [](void* state, ttx_abstract result) {
              (*static_cast<Receiver*>(state))(Abstract(result));
            }));
  }
  template <typename Owner>
  static auto provide(const Owner& owner) -> Parse {
    static const Operations operations{
      *Abstract::provide(owner).get_abi().operations,
      [](const void* source, tetrodotoxin_source_cursor* state,
         ttx_abstract diagnostics, void* receiver,
         void (*receive)(void*, ttx_abstract)) -> ttx_binding_status {
        if (!state || !Stream::accept(state->stream) ||
            !Abstract::accept(diagnostics) || !receive) {
          return TTX_BINDING_REJECTED;
        }
        Cursor cursor(*state);
        if (!cursor.is_valid()) {
          return TTX_BINDING_REJECTED;
        }
        return Abstract(diagnostics)
            .bind<Diagnostics>()
            .visit(
                [&](Diagnostics sink) -> ttx_binding_status {
                  auto observe = [&](Abstract result) {
                    receive(receiver, result.get_abi());
                  };
                  auto status = static_cast<const Owner*>(source)->parse(
                      cursor, sink, observe);
                  return static_cast<ttx_binding_status>(status);
                },
                [](Ttx::Semantic::Negotiation::Binding::Failure failure) {
                  return static_cast<ttx_binding_status>(failure);
                });
      }};
    return Parse({&owner, &operations});
  }
};
}  // namespace Tetrodotoxin::Dialects::Source::Capabilities

TTX_DATA_RECORD(
    tetrodotoxin_source_parse_ops,
    TTX_DATA_MEMBER(tetrodotoxin_source_parse_ops, abstract),
    TTX_DATA_MEMBER(tetrodotoxin_source_parse_ops, parse));
TTX_DATA_RECORD(
    tetrodotoxin_source_parse,
    TTX_DATA_MEMBER(tetrodotoxin_source_parse, source),
    TTX_DATA_MEMBER(tetrodotoxin_source_parse, operations));
