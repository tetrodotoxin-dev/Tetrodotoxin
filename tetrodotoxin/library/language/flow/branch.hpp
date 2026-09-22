// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/statement.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// Branch owns one complete authored `if` or `while` statement. Its condition
// keeps the complete Pack even though control flow selects the first value.
// Each body is a real nested Block with the enclosing Block as lexical parent.
class Branch : public Tetrodotoxin::Source::Abstract {
 public:
  enum class Kind : U8 {
    If,
    While,
  };

  TTX_CONTRACT(Branch, Tetrodotoxin::Source::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Kind kind,
      Model::Pack& condition,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Branch&;

  auto complete_body(Block& selected) -> Bool;

  auto complete_alternate(Statement selected) -> Bool;

  auto complete_anchor(Tetrodotoxin::Source::Lexical::Anchor selected) -> void;

  Branch(const Branch&) = delete;
  Branch(Branch&&) = delete;
  auto operator=(const Branch&) -> Branch& = delete;
  auto operator=(Branch&&) -> Branch& = delete;

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Scope& lexical_context,
      const Model::Type& access_scope) -> Bool;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void;

  auto reaches_next_statement() const -> Bool;

  TTX_NAME("Branch"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_kind() const -> Kind { return kind; }

  constexpr auto get_condition() const -> const Model::Pack& {
    return condition.get();
  }

  constexpr auto get_body() const -> const Block& { return body->get(); }

  constexpr auto get_alternate() const
      -> Perimortem::Core::Option<const Statement&> {
    return alternate.visit(
        []() -> Perimortem::Core::Option<const Statement&> { return {}; },
        [](const Statement& selected)
            -> Perimortem::Core::Option<const Statement&> { return selected; });
  }

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

 private:
  constexpr Branch(
      Kind kind,
      Model::Pack& condition,
      Tetrodotoxin::Source::Lexical::Anchor anchor)
      : kind(kind), condition(condition), anchor(anchor) {}

  Kind kind;
  Tetrodotoxin::Source::PackReference<Model::Pack> condition;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<Block>> body;
  Perimortem::Core::Option<Statement> alternate;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
