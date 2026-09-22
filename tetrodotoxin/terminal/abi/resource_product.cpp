// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/abi/resource_product.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/linker/elf/object.hpp"
#include "tetrodotoxin/package/resource.hpp"
#include "tetrodotoxin/terminal/abi/symbol.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

auto Terminal::Abi::ResourceProduct::compile(
    Memory::Allocator::Arena& arena,
    const Package::Resources& resources,
    Core::View::Bytes package,
    Core::View::Bytes artifact) -> Core::Option<ResourceProduct> {
  Linker::Elf::Object object;
  Memory::Managed::Vector<Unit::Binding> bindings(arena);
  Unit unit(package, "resources"_view, artifact);
  for (const Tetrodotoxin::Source::Reference<Package::Resource>& retained :
       resources.get_values()) {
    const Package::Resource& resource = retained.get();
    Symbol symbol(arena, resource, Symbol::Kind::ReadOnly, unit);
    Memory::Managed::Bytes end_symbol(arena, symbol.get_view());
    end_symbol.concat("_end"_view);
    BAIL_IF(!object.add_read_only(
        symbol.get_view(), end_symbol.get_view(), resource.get_value()));
    bindings.insert(Unit::Binding(resource, symbol.get_view()));
  }

  auto bytes = object.build();
  BAIL_IF(!bytes);
  return ResourceProduct(
      static_cast<Memory::Dynamic::Bytes&&>(*bytes), bindings.get_view());
}
