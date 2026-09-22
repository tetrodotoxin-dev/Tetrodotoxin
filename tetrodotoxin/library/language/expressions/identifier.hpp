// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/token.hpp"

namespace Tetrodotoxin::Library::Language::Expressions {

// Identifier is one authored root name Expression. It retains the exact Token
// and a view of the source spelling in the shared transaction Arena, then
// delays binding until the Type defining pass is complete. A Type result stays
// outside value flow while an Addressable result keeps its ordinary value Type.
// Both remain opaque until link selects them.
class Identifier : public Expression {
 public:
  TTX_CONTRACT(Identifier, Expression);

  static auto create_authored(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Tetrodotoxin::Source::Lexical::Token token,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Identifier& {
    Perimortem::Core::View::Bytes name =
        token.caculate_text(cursor.get_source_text());
    return Expression::create_authored<Identifier>(
        cursor.get_arena(), anchor, [&](auto authored) -> Identifier {
          return Identifier(token, name, lexical_context, authored);
        });
  }

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name) -> Identifier& {
    return Expression::create_synthetic<Identifier>(
        arena, [&](auto source) -> Identifier {
          return Identifier(
              {}, name, Tetrodotoxin::Source::Unknown::get_unknown(), source);
        });
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

  TTX_NAME(name);

  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override;

  auto get_type() const -> const Tetrodotoxin::Source::Abstract& override;

  auto get_result() const -> const Tetrodotoxin::Source::Abstract& override;

  // A malformed following operator may keep this Identifier outside a
  // retained Statement. Its authored context can still answer the strongest
  // source ordered binding without manufacturing a completed result edge.
  auto resolve_authored() const -> const Tetrodotoxin::Source::Abstract&;

  constexpr auto get_token() const -> Tetrodotoxin::Source::Lexical::Token { return token; }

 private:
  constexpr Identifier(
      Tetrodotoxin::Source::Lexical::Token token,
      Perimortem::Core::View::Bytes name,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Expression(anchor),
        token(token),
        name(name),
        lexical_context(lexical_context) {}

  Tetrodotoxin::Source::Lexical::Token token;
  Perimortem::Core::View::Bytes name;
  Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract> lexical_context;
  Perimortem::Core::Option<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>>
      result;
};

}  // namespace Tetrodotoxin::Library::Language::Expressions
