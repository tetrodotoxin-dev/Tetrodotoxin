// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Implementation is the explicit Library value that erases one accepted
// Object behind a semantic requirement. It retains no member inventory or
// runtime table. The candidate owner answers satisfaction and the ABI Terminal
// derives the immutable Projection used by native code.
class Implementation : public Model::Type {
 public:
  TTX_CONTRACT(Implementation, Model::Type);

  constexpr Implementation(
      Perimortem::Core::View::Bytes name,
      const Tetrodotoxin::Source::Type& requirement)
      : name(name), requirement(requirement) {}

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto accepts(const Model::Pack& source) const -> Bool override;

  auto validate_layout(Tetrodotoxin::Source::Lexical::Cursor& cursor) const -> Bool override;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  constexpr auto get_requirement() const -> const Tetrodotoxin::Source::Type& {
    return requirement.get();
  }

 private:
  Perimortem::Core::View::Bytes name;
  Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type> requirement;
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Carries one accepted Object with its target Projection."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
