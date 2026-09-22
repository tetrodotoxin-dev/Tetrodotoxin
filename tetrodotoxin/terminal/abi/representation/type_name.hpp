// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/terminal/abi/unit.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Terminal::Abi::Representation {

// TypeName gives one completed semantic Type its exact C carrier spelling. The
// C and C++ interface producers share this value, keeping every generated
// declaration on the same Package and member route.
class TypeName {
 public:
  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      const Unit& unit,
      const Tetrodotoxin::Source::Type& type,
      Perimortem::Core::View::Bytes inherited_package = {},
      Perimortem::Core::View::Bytes inherited_member = {})
      -> Perimortem::Core::Option<TypeName>;

  constexpr auto get_type() const -> const Tetrodotoxin::Source::Type& {
    return type.get();
  }

  constexpr auto get_value() const -> Perimortem::Core::View::Bytes {
    return value;
  }

  constexpr auto get_package() const -> Perimortem::Core::View::Bytes {
    return package;
  }

  constexpr auto get_member() const -> Perimortem::Core::View::Bytes {
    return member;
  }

 private:
  constexpr TypeName(
      const Tetrodotoxin::Source::Type& type,
      Perimortem::Core::View::Bytes value,
      Perimortem::Core::View::Bytes package,
      Perimortem::Core::View::Bytes member)
      : type(type), value(value), package(package), member(member) {}

  Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type> type;
  Perimortem::Core::View::Bytes value;
  Perimortem::Core::View::Bytes package;
  Perimortem::Core::View::Bytes member;
};

}  // namespace Tetrodotoxin::Terminal::Abi::Representation
