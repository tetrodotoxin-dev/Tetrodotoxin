// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/lexical/token.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Swizzle selects and reorders named values from one receiver Pack. A named
// Pack contributes the real producer retained at each selected source index.
// One producer with several results may therefore supply distinct slots. A
// scalar Expression may instead contribute the named Addressables of its
// output Type, in which case each selected slot is a real Address Expression
// bound to that receiver. The result is positional Pack flow and never an
// eagerly materialized Type.
class Swizzle : public Expression {
 public:
  TTX_CONTRACT(Swizzle, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Language::Model::Pack& receiver,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::Lexical::Token> name_tokens,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Swizzle&;

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME("Swizzle"_view);

  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override;
  auto get_type() const -> const Tetrodotoxin::Source::Abstract& override;
  auto get_value_type(Count index) const
      -> const Tetrodotoxin::Source::Abstract& override;
  auto get_layout() const -> const Tetrodotoxin::Source::Layout& override;
  auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;
  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void override;

  constexpr auto get_receiver() const -> const Language::Model::Pack& {
    return receiver;
  }

  constexpr auto get_projections() const { return projections.get_view(); }

  constexpr auto get_selections() const { return selections.get_view(); }

 private:
  Swizzle(
      Perimortem::Memory::Allocator::Arena& domain,
      Language::Model::Pack& receiver,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::Lexical::Token> name_tokens,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Expression(anchor),
        domain(domain),
        receiver(receiver),
        name_tokens(name_tokens),
        names(names),
        selections(domain),
        projections(domain) {}

  Perimortem::Memory::Allocator::Arena& domain;
  Language::Model::Pack& receiver;
  Perimortem::Core::View::Vector<Tetrodotoxin::Source::Lexical::Token> name_tokens;
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names;
  Perimortem::Memory::Managed::Vector<Count> selections;
  Perimortem::Memory::Managed::Vector<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>>
      projections;
  Perimortem::Core::Option<const Tetrodotoxin::Source::Layout&> output;
};

}  // namespace Tetrodotoxin::Library::Language::Access
