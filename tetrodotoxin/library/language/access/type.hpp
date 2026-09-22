// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Type is one postfix `:: Name` Expression. It retains the receiver and exact
// authored Token without binding during parsing. Linking evaluates the
// receiver result and selects the next context through that owner. Intermediate
// Package, Monograph, and namespace contexts remain available to another `::`.
// A terminal Type can enter Static invocation or declaration flow, while a
// context owned value such as an Enumeration case enters ordinary value flow.
class Type : public Expression {
 public:
  TTX_CONTRACT(Type, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Tetrodotoxin::Source::Lexical::Token token,
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Type&;

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME(name);

  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override;
  auto get_type() const -> const Tetrodotoxin::Source::Abstract& override;
  auto get_result() const -> const Tetrodotoxin::Source::Abstract& override;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  auto visit_concepts(Tetrodotoxin::Source::Abstract::Visitor visitor) const
      -> void override;

  // A following incomplete postfix may leave this access outside a retained
  // Statement. The receiver still owns enough authored context to answer the
  // strongest currently available selection without completing this node.
  auto resolve_authored() const -> const Tetrodotoxin::Source::Abstract&;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void override;

  constexpr auto get_receiver() const -> const Model::Pack& { return receiver; }
  constexpr auto get_token() const -> Tetrodotoxin::Source::Lexical::Token { return token; }

 private:
  constexpr Type(
      Model::Pack& receiver,
      Tetrodotoxin::Source::Lexical::Token token,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Expression(anchor), receiver(receiver), token(token), name(name) {}

  Model::Pack& receiver;
  Tetrodotoxin::Source::Lexical::Token token;
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Option<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>>
      selected;
};

}  // namespace Tetrodotoxin::Library::Language::Access
