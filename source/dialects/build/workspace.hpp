// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once
#include "tetrodotoxin/dialects/build/workspace.h"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Dialects::Build {

// This contract adds terminal destination selection to ordinary Abstract
// navigation. Source roots keep their own identities, interfaces and policies.
class Workspace : public Ttx::Concept::Abstract {
 public:
  using Api = tetrodotoxin_build_workspace;
  static constexpr auto contract_id = Perimortem::System::Uuid(
      TETRODOTOXIN_BUILD_WORKSPACE_ID_HIGH,
      TETRODOTOXIN_BUILD_WORKSPACE_ID_LOW);
  static auto accept(Api value) -> Bool {
    return value.operations && value.operations->output_directory &&
           Abstract::accept({value.source, &value.operations->abstract});
  }
  explicit Workspace(Api value)
      : Abstract(value.source, value.operations->abstract) {}
  auto get_abi() const -> Api {
    const auto value = Abstract::get_abi();
    return {
      value.source, reinterpret_cast<const tetrodotoxin_build_workspace_ops*>(
                        value.operations)};
  }
  auto get_output_directory() const -> Perimortem::Core::View::Bytes {
    const auto value = get_abi();
    const auto path = value.operations->output_directory(value.source);
    return {path.data, path.size};
  }
};

}  // namespace Tetrodotoxin::Dialects::Build

TTX_DATA_RECORD(
    tetrodotoxin_build_workspace_ops,
    TTX_DATA_MEMBER(tetrodotoxin_build_workspace_ops, abstract),
    TTX_DATA_MEMBER(tetrodotoxin_build_workspace_ops, output_directory));
TTX_DATA_RECORD(
    tetrodotoxin_build_workspace,
    TTX_DATA_MEMBER(tetrodotoxin_build_workspace, source),
    TTX_DATA_MEMBER(tetrodotoxin_build_workspace, operations));
