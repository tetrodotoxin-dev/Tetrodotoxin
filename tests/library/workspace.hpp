// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/record.hpp"

#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"

namespace Validation {

// The caller keeps Library alive beyond this Toolchain and every Workspace
// borrowing it. Fixtures supply the dependency just as a product host does.
inline auto create_library_toolchain(Tetrodotoxin::Library::Dialect& library)
    -> Perimortem::Memory::Dynamic::Record<
        Tetrodotoxin::Environment::Toolchain> {
  Perimortem::Memory::Dynamic::Record<Tetrodotoxin::Environment::Toolchain>
      toolchain;
  toolchain->install(library);
  return toolchain;
}

inline auto get_library_dialect(Tetrodotoxin::Environment::Toolchain& toolchain)
    -> Tetrodotoxin::Library::Dialect& {
  return static_cast<Tetrodotoxin::Library::Dialect&>(
      *toolchain.find("Library"_view));
}

inline auto retains_library_source(
    const Tetrodotoxin::Environment::Workspace& workspace,
    Perimortem::Core::View::Bytes semantic_name) -> Bool {
  return workspace.resolve_concept(semantic_name)
      .is<Tetrodotoxin::Library::Language::Monograph>();
}

}  // namespace Validation
