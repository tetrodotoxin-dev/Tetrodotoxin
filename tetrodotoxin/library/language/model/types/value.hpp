// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/source/unknown.hpp"

namespace Tetrodotoxin::Library::Language::Model::Types {

// Value is the Library scalar representation protocol. Width is semantic bit
// precision, while size and alignment are target storage facts. Keeping them
// distinct prevents category eligibility from implying promotion or packing.
class Value : public Model::Type {
 public:
  TTX_CONTRACT(Value, Model::Type);

  // Folding may retain the selected Type while producing an incompatible
  // Constant carrier. The scalar domain owns that proof so every consumer can
  // reject malformed folded values without enumerating Constant subclasses.
  virtual auto accepts_constant(const Tetrodotoxin::Source::Abstract&) const
      -> Bool = 0;

  // Builtin scalars carry the same Library meaning in every source Monograph.
  // Cross source flow therefore admits the matching category and precision
  // without treating an arbitrary authored Type or physical representation as
  // interchangeable.
  auto accepts(const Model::Pack& source) const -> Bool override;

  // Builtin scalar identities are Monograph local, while their category and
  // precision are language wide meaning. This relation lets composed Dialects
  // preserve one operand identity without treating authored Types or target
  // storage as interchangeable.
  auto is_equivalent(const Value& source) const -> Bool;

  auto create_supplied(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Model::Pack& source,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto create_supplied_restored(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& source,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope)
      const -> Perimortem::Core::Option<Model::Pack&> override;

  virtual constexpr auto get_width() const -> Count = 0;
  virtual constexpr auto get_size() const -> Count = 0;
  virtual constexpr auto get_alignment() const -> Count = 0;
};

}  // namespace Tetrodotoxin::Library::Language::Model::Types
