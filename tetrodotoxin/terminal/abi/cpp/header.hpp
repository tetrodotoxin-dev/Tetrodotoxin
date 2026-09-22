// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/terminal/abi/export.hpp"
#include "tetrodotoxin/terminal/abi/representation/type.hpp"
#include "tetrodotoxin/terminal/abi/unit.hpp"

namespace Tetrodotoxin::Terminal::Abi::Cpp {

// Header is the generated C++ Package surface for one completed Library unit.
// Its header follows semantic Package and Type routes, while its implementation
// forwards every operation through the matching generated C symbol.
class Header {
 public:
  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
      const Tetrodotoxin::Terminal::Abi::Unit& unit,
      Perimortem::Core::View::Vector<Tetrodotoxin::Terminal::Abi::Export>
          exports) -> Perimortem::Core::Option<Header>;

  constexpr auto get_header() const -> Perimortem::Core::View::Bytes {
    return header;
  }

  constexpr auto get_source() const -> Perimortem::Core::View::Bytes {
    return source;
  }

 private:
  constexpr Header(
      Perimortem::Core::View::Bytes header,
      Perimortem::Core::View::Bytes source)
      : header(header), source(source) {}

  Perimortem::Core::View::Bytes header;
  Perimortem::Core::View::Bytes source;
};

}  // namespace Tetrodotoxin::Terminal::Abi::Cpp
