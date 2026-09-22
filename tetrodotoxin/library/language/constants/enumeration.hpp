// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Enumeration is one immutable value in an exact authored Enumeration domain.
// The raw storage value remains independent from the optional case Aliases.
class Enumeration : public Tetrodotoxin::Library::Language::Constant {
 public:
  TTX_CONTRACT(Enumeration, Tetrodotoxin::Library::Language::Constant);

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Enumeration& type,
      U64 value) -> Enumeration& {
    return Constant::create_synthetic<Enumeration>(
        domain, [&](auto source) -> Enumeration {
          return Enumeration(domain, type, value, source);
        });
  }

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Enumeration& type,
      U64 value,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Enumeration& {
    return Constant::create_authored<Enumeration>(
        domain, anchor, [&](auto source) -> Enumeration {
          return Enumeration(domain, type, value, source);
        });
  }

  constexpr auto get_type() const -> const Types::Enumeration& override {
    return type;
  }

  constexpr auto get_value() const -> U64 { return value; }

  auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name.get_view();
  }

  constexpr auto equals(const Tetrodotoxin::Library::Language::Constant& rhs)
      const -> Bool override {
    return rhs.visit<Enumeration>(
        [this, &rhs](const Enumeration& selected) {
          return has_same_type(rhs) && value == selected.value ? True : False;
        },
        [](const Tetrodotoxin::Source::Abstract&) { return False; });
  }

 private:
  Enumeration(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Enumeration& type,
      U64 value,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Tetrodotoxin::Library::Language::Constant(anchor),
        type(type),
        value(value),
        name(domain) {
    Perimortem::Serialization::Stream::Textual<
        Perimortem::Memory::Managed::Bytes>
        output(name);
    output << type.get_name() << "::"_view << value;
  }

  const Types::Enumeration& type;
  U64 value;
  Perimortem::Memory::Managed::Bytes name;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
