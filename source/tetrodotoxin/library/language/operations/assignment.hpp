// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Operations {

// Assignment is the lowest precedence `=` Expression operator. It retains one
// writable target and the complete Pack written to it. The empty output Layout
// records an effect without fabricating a result value.
class Assignment : public Expression {
 public:
  TTX_CONTRACT(Assignment, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& target,
      Model::Pack& source,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Assignment&;

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void override;

  TTX_NAME("Assignment"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_type() const -> const Tetrodotoxin::Source::Abstract& override {
    return Tetrodotoxin::Source::Unknown::get_unknown();
  }

  auto get_value_type(Count index) const
      -> const Tetrodotoxin::Source::Abstract& override;
  auto get_layout() const -> const Tetrodotoxin::Source::Layout& override;
  auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;

  constexpr auto get_target() const -> const Expression& { return target; }

  constexpr auto get_source() const -> const Model::Pack& { return source; }

 private:
  constexpr Assignment(
      Expression& target,
      Model::Pack& source,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Expression(anchor), target(target), source(source) {}

  Expression& target;
  Model::Pack& source;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Operations
