// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/types/value.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Library::Language::Expressions {

// Conversion is the explicit scalar construction selected by `new[Target]`.
// It keeps the real source Pack and destination Value Type as graph edges, so
// folding and Terminals can apply the same total conversion without treating
// target representation as language meaning. Integer bounds are saturated,
// real to integer conversion truncates toward zero, and NaN becomes zero.
class Conversion : public Expression {
 public:
  TTX_CONTRACT(Conversion, Expression);

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      const Model::Types::Value& target,
      Model::Pack& source) -> Conversion&;

  static auto accepts(
      const Model::Types::Value& target,
      const Model::Pack& source) -> Bool;

  TTX_NAME("Conversion"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_type() const -> const Model::Types::Value& override {
    return target.get();
  }

  constexpr auto get_source() const -> const Model::Pack& {
    return source.get();
  }

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;

  auto link_restored(
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void override;

 protected:
  auto evaluate() -> Perimortem::Utility::Result<
      Perimortem::Core::Option<Model::Pack&>,
      Expression::Error> override;

 private:
  constexpr Conversion(
      Perimortem::Memory::Allocator::Arena& arena,
      const Model::Types::Value& target,
      Model::Pack& source)
      : Expression({}), arena(arena), target(target), source(source) {}

  Perimortem::Memory::Allocator::Arena& arena;
  Tetrodotoxin::Source::Reference<const Model::Types::Value> target;
  Tetrodotoxin::Source::PackReference<Model::Pack> source;
};

}  // namespace Tetrodotoxin::Library::Language::Expressions
