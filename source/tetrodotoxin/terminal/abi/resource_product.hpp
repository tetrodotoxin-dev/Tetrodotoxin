// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/package/resources.hpp"
#include "tetrodotoxin/terminal/abi/unit.hpp"

namespace Tetrodotoxin::Terminal::Abi {

// ResourceProduct projects the Package owned Resource inventory into one
// native object. Each binding points back to the real Resource identity, which
// lets member Terminals import these symbols without copying bytes or building
// another resource table.
class ResourceProduct {
 public:
  static auto compile(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Package::Resources& resources,
      Perimortem::Core::View::Bytes package,
      Perimortem::Core::View::Bytes artifact)
      -> Perimortem::Core::Option<ResourceProduct>;

  constexpr auto get_object() const -> Perimortem::Core::View::Bytes {
    return object;
  }

  constexpr auto get_bindings() const
      -> Perimortem::Core::View::Vector<Unit::Binding> {
    return bindings;
  }

 private:
  ResourceProduct(
      Perimortem::Memory::Dynamic::Bytes&& object,
      Perimortem::Core::View::Vector<Unit::Binding> bindings)
      : object(static_cast<Perimortem::Memory::Dynamic::Bytes&&>(object)),
        bindings(bindings) {}

  Perimortem::Memory::Dynamic::Bytes object;
  Perimortem::Core::View::Vector<Unit::Binding> bindings;
};

}  // namespace Tetrodotoxin::Terminal::Abi
