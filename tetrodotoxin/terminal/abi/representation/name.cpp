// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/abi/representation/name.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/terminal/abi/symbol.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;

Tetrodotoxin::Terminal::Abi::Representation::Name::Name(
    Memory::Allocator::Arena& arena,
    const Tetrodotoxin::Source::Abstract& semantic,
    Kind kind) {
  Memory::Managed::Bytes output(arena);
  switch (kind) {
  case Kind::ImplementationType:
    output.concat("ttx.implementation."_view);
    break;
  case Kind::OptionType:
    output.concat("ttx.option."_view);
    break;
  case Kind::ResultType:
    output.concat("ttx.result."_view);
    break;
  case Kind::StructureType:
    output.concat("ttx.struct."_view);
    break;
  case Kind::ObjectType:
    output.concat("ttx.object."_view);
    break;
  case Kind::ObjectFinalizer:
    output.concat("__ttx_object_finalize_"_view);
    break;
  case Kind::ObjectDescriptor:
    output.concat("__ttx_object_descriptor_"_view);
    break;
  }
  Tetrodotoxin::Terminal::Abi::Symbol path(
      arena, semantic, Tetrodotoxin::Terminal::Abi::Symbol::Kind::Path);
  output.concat(path.get_view());
  value = output.get_view();
}
