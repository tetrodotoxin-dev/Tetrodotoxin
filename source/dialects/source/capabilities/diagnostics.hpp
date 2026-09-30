// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialects/source/anchor.hpp"
#include "tetrodotoxin/dialects/source/capabilities/diagnostics.h"
#include "tetrodotoxin/dialects/source/snapshot.hpp"

namespace Tetrodotoxin::Dialects::Source::Capabilities {

// Diagnostics is a borrowed reporting service. Copying it neither retains the
// collector nor grants access to its allocator or presentation machinery.
class Diagnostics : public Ttx::Concept::Abstract {
 public:
  using Api = tetrodotoxin_source_diagnostics;
  using Operations = tetrodotoxin_source_diagnostics_ops;
  static constexpr auto contract_id = Perimortem::System::Uuid(
      TETRODOTOXIN_SOURCE_DIAGNOSTICS_ID_HIGH,
      TETRODOTOXIN_SOURCE_DIAGNOSTICS_ID_LOW);
  static auto accept(Api value) -> Bool {
    return value.operations && value.operations->report &&
           value.operations->count &&
           Abstract::accept({value.source, &value.operations->abstract});
  }
  explicit constexpr Diagnostics(Api value)
      : Abstract(value.source, value.operations->abstract) {}
  constexpr auto get_abi() const -> Api {
    auto value = Abstract::get_abi();
    return {
      value.source, reinterpret_cast<const Operations*>(value.operations)};
  }
  auto report(
      tetrodotoxin_source_snapshot input,
      Anchor anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) const -> void {
    auto value = get_abi();
    value.operations->report(
        value.source, input, anchor.get_abi(),
        {message.get_data(), message.get_size()},
        {hint.get_data(), hint.get_size()});
  }
  auto get_size() const -> Count {
    auto value = get_abi();
    return value.operations->count(value.source);
  }
  template <typename Owner>
  static auto provide(Owner& owner) -> Diagnostics {
    static const Operations operations{
      *Abstract::provide(owner).get_abi().operations,
      [](const void* source, tetrodotoxin_source_snapshot input,
         tetrodotoxin_source_anchor anchor, perimortem_view_bytes message,
         perimortem_view_bytes hint) {
        const_cast<Owner*>(static_cast<const Owner*>(source))
            ->report(
                input, Anchor(anchor), {message.data, message.size},
                {hint.data, hint.size});
      },
      [](const void* source) -> Count {
        return static_cast<const Owner*>(source)->get_size();
      }};
    return Diagnostics({&owner, &operations});
  }
};
}  // namespace Tetrodotoxin::Dialects::Source::Capabilities

TTX_DATA_RECORD(
    tetrodotoxin_source_diagnostics_ops,
    TTX_DATA_MEMBER(tetrodotoxin_source_diagnostics_ops, abstract),
    TTX_DATA_MEMBER(tetrodotoxin_source_diagnostics_ops, report),
    TTX_DATA_MEMBER(tetrodotoxin_source_diagnostics_ops, count));
TTX_DATA_RECORD(
    tetrodotoxin_source_diagnostics,
    TTX_DATA_MEMBER(tetrodotoxin_source_diagnostics, source),
    TTX_DATA_MEMBER(tetrodotoxin_source_diagnostics, operations));
