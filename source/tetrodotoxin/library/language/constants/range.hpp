// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Range is the exact empty value of one lazy integer Range Type. It carries no
// fabricated element, storage view, or iterator identity.
class Range : public Tetrodotoxin::Library::Language::Constant {
 public:
  TTX_CONTRACT(Range, Tetrodotoxin::Library::Language::Constant);

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Range& type) -> Range& {
    return Constant::create_synthetic<Range>(
        domain, [&](auto source) -> Range { return Range(type, source); });
  }

  constexpr auto get_type() const -> const Types::Range& override {
    return type;
  }

  TTX_NAME("[]"_view);

  constexpr auto is_empty() const -> Bool { return True; }

  constexpr auto equals(const Tetrodotoxin::Library::Language::Constant& rhs)
      const -> Bool override {
    return rhs.is<Range>() && has_same_type(rhs);
  }

 private:
  constexpr Range(
      const Types::Range& type,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Tetrodotoxin::Library::Language::Constant(anchor), type(type) {}

  const Types::Range& type;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
