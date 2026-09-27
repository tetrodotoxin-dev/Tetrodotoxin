// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/foreign.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/linker/fingerprint.hpp"
#include "tetrodotoxin/terminal/abi/export.hpp"
#include "tetrodotoxin/terminal/abi/representation/type.hpp"
#include "tetrodotoxin/terminal/abi/unit.hpp"

namespace Tetrodotoxin::Terminal::Abi::C {

// Header is the generated C interface for one completed publication set. Its
// bytes share the compilation Arena and remain valid with the other products.
class Header {
 public:
  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
      const Tetrodotoxin::Library::Language::Monograph& monograph,
      const Tetrodotoxin::Terminal::Abi::Unit& unit,
      Perimortem::Core::View::Vector<Tetrodotoxin::Terminal::Abi::Export>
          exports,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<
          const Tetrodotoxin::Library::Language::Model::Type>> roots = {})
      -> Perimortem::Core::Option<Header>;

  static auto identify(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes owner,
      Tetrodotoxin::Linker::Fingerprint fingerprint)
      -> Perimortem::Core::Option<Header>;

  constexpr auto get_view() const -> Perimortem::Core::View::Bytes {
    return value;
  }

 private:
  constexpr Header(Perimortem::Core::View::Bytes value) : value(value) {}

  Perimortem::Core::View::Bytes value;
};

}  // namespace Tetrodotoxin::Terminal::Abi::C
