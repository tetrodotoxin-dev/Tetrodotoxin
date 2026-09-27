// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Propagate is a special control flow access (post fix ?) that will `BAIL_IF`
// in the middle of an access expression allowing it to be chained.
//
// It can be used for scenarios where it would be verbose to break up an
// expression chain to check `if !option : return`, but it desugars to the same
// resulting control flow. It does carry the explicit semantics through the
// expression chain, but we don't currently perform any meaningful optimizations
// by utilizing this fact.
class Propagate : public Expression {
 public:
  TTX_CONTRACT(Propagate, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Propagate&;

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME("Propagate"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto get_type() const -> const Tetrodotoxin::Source::Abstract& override;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void override;

  constexpr auto get_receiver() const -> const Model::Pack& { return receiver; }

  constexpr auto get_escape() const -> const Model::Pack& {
    return escape.get();
  }

 protected:
  auto evaluate() -> Perimortem::Utility::Result<
      Perimortem::Core::Option<Model::Pack&>,
      Expression::Error> override;

 private:
  class ErrorEscape : public Expression {
   public:
    TTX_CONTRACT(ErrorEscape, Expression);
    TTX_NAME("Propagation error"_view);
    TTX_EMPTY_DOCUMENTATION();

    constexpr auto get_type() const -> const Model::Type& override {
      return type;
    }

   private:
    friend class Propagate;

    constexpr explicit ErrorEscape(const Model::Type& type)
        : Expression({}), type(type) {}

    const Model::Type& type;
  };

  constexpr Propagate(
      Model::Pack& receiver,
      Model::Pack& empty_escape,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Expression(anchor), receiver(receiver), escape(empty_escape) {}

  Model::Pack& receiver;
  Tetrodotoxin::Source::PackReference<Model::Pack> escape;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Model::Type>>
      receiver_type;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Model::Type>>
      continuation_type;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Model::Type>>
      error_type;
};

}  // namespace Tetrodotoxin::Library::Language::Access
