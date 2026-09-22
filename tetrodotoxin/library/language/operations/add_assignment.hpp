// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// AddAssignment owns the explicit `+=` read, modify, and write operator. Its
// target and right operand are the only two graph edges, so lowering never has
// to deduplicate a hidden Add expression before writing the selected address.
class AddAssignment : public Expression {
 public:
  TTX_CONTRACT(AddAssignment, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& target,
      Model::Pack& right,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> AddAssignment&;

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void override;

  TTX_NAME("AddAssignment"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_type() const -> const Tetrodotoxin::Source::Abstract& override {
    return Tetrodotoxin::Source::Unknown::get_unknown();
  }

  auto get_value_type(Count index) const
      -> const Tetrodotoxin::Source::Abstract& override;
  auto get_layout() const -> const Tetrodotoxin::Source::Layout& override;
  auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;

  constexpr auto get_target() const -> const Expression& { return target; }

  constexpr auto get_right() const -> const Model::Pack& { return right; }

 private:
  constexpr AddAssignment(
      Expression& target,
      Model::Pack& right,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Expression(anchor), target(target), right(right) {}

  Expression& target;
  Model::Pack& right;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Operations
