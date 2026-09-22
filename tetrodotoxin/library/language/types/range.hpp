// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Range is one lazy integer sequence Type. It retains the exact element edge
// without claiming contiguous storage, state, or ownership of its Generic key.
class Range : public Model::Type {
 public:
  TTX_CONTRACT(Range, Model::Type);

  constexpr Range(
      Perimortem::Core::View::Bytes name,
      const Model::Type& element)
      : name(name), element(element) {}

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto accepts_iteration(const Tetrodotoxin::Source::Layout& bindings) const
      -> Bool override;

  constexpr auto get_element_type() const -> const Model::Type& {
    return element;
  }

  constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> override {
    return element.get_declaration_anchor();
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& element;
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Provides a lazy ascending integer sequence."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
