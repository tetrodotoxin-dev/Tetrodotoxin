// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/package/resource.hpp"
#include "tetrodotoxin/package/storage.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Package {

// Owns contextual resource identities for one Package Monograph. Monograph
// construction has its source transaction Arena but not physical Package
// Storage, so Workspace connects this owner to confined Storage while it walks
// the source-local Type graph.
//
// While connected, complete resource routes read through that one
// Storage and cache one Arena stable Resource or Error identity. The Package
// import operation seals Resources before it links or finalizes the graph.
// Sealing clears only
// the borrowed Storage pointer, so cached identities remain valid for the
// Monograph lifetime while a new route resolves to None after sealing.
//
// A Resources connection can be used only once. Sealing never permits another
// Storage to be selected, and source free construction seals before publishing
// its Monograph.
class Resources {
 private:
  enum class Stage : U8 {
    Pending,
    Connected,
    Sealed,
  };

 public:
  Resources(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<Resource>>
          restored = {},
      Bool sealed = False);

  auto connect(Storage& storage) -> Bool;
  auto seal() -> void;

  auto resolve(Perimortem::Core::View::Bytes logical_route)
      -> const Tetrodotoxin::Source::Abstract&;

  constexpr auto get_values() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<Resource>> {
    return values;
  }

 private:
  Perimortem::Memory::Allocator::Arena& domain;
  Storage* storage;
  Stage stage;
  Perimortem::Memory::Managed::Map<Perimortem::Core::View::Bytes, Resource&>
      resource_cache;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Tetrodotoxin::Language::Error&>
          error_cache;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Source::Reference<Resource>> values;
};

}  // namespace Tetrodotoxin::Package
