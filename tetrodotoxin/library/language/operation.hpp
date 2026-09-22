// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Library::Language {

// Operation is the Expression base for executable value operations. It owns
// authored input order and the one exact result Type selected after every
// child links. A concrete operation supplies its legality and
// Tetrodotoxin::Library::Language::Constant evaluation without asking Parser to
// interpret semantic Types.
class Operation : public Expression {
 public:
  TTX_CONTRACT(Operation, Expression);

  auto get_type() const -> const Tetrodotoxin::Source::Abstract& override;

  auto get_documentation() const
      -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void override;

  // Operations retain their exact authored scalar input order. Consumers visit
  // those real Pack flows without reconstructing another input model.
  constexpr auto get_inputs() const -> Perimortem::Core::View::Vector<
      Tetrodotoxin::Source::PackReference<Model::Pack>> {
    return inputs.get_view();
  }

 protected:
  Operation(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>>
          inputs,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor);

  // Current Operations are scalar: Parser proves that each operand Pack is
  // the exact Expression retained here. This vector is the canonical ordered
  // evaluation inventory. A future operation over flow with several values must
  // own that Pack shape rather than flatten it into this scalar contract.
  auto fold_input(Count index) -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Constant&>, Expression::Error>;

  // Concrete evaluation runs only after the ordered traversal completed.
  // This observation unwraps that cached result without changing the
  // authored edge or starting another computation.
  auto get_folded_input(Count index) -> Perimortem::Core::Option<Constant&>;

  virtual auto evaluate_constants(Perimortem::Memory::Allocator::Arena& domain)
      -> Perimortem::Utility::Result<
          Perimortem::Core::Option<Tetrodotoxin::Library::Language::Constant&>,
          Expression::Error> = 0;

  virtual auto reaches_next_input(Count folded_input, const Constant& folded)
      const -> Bool;

  // Type selection runs during link, where the exact lexical context and every
  // input edge have completed.
  virtual auto select_type(const Tetrodotoxin::Source::Abstract& context) const
      -> Perimortem::Core::Option<const Model::Type&> = 0;

  auto evaluate() -> Perimortem::Utility::Result<
      Perimortem::Core::Option<Model::Pack&>,
      Expression::Error> override;

 private:
  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>>
      inputs;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Model::Type>>
      result_type;
};

}  // namespace Tetrodotoxin::Library::Language

// Binary Operations share their category proof, construction surface, and
// canonical semantic name while keeping parsing and evaluation visible.
#define BINARY_OP_CONTRACT(type)                                              \
  TTX_CONTRACT(type, Operation);                                              \
  static auto create_authored(                                                \
      Perimortem::Memory::Allocator::Arena& domain, Model::Pack& left,        \
      Model::Pack& right, Tetrodotoxin::Source::Lexical::Anchor anchor) -> type&;              \
  static auto create_synthetic(                                               \
      Perimortem::Memory::Allocator::Arena& domain, Model::Pack& left,        \
      Model::Pack& right) -> type&;                                           \
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override { \
    return #type##_view;                                                      \
  }

// Concrete Operations own distinct semantics while sharing the same authored
// and synthetic construction path over one retained operand inventory.
#define TTX_BINARY_OP(type)                                                 \
  auto Tetrodotoxin::Library::Language::Operations::type::create_authored(  \
      Perimortem::Memory::Allocator::Arena& domain, Model::Pack& left,      \
      Model::Pack& right, Tetrodotoxin::Source::Lexical::Anchor anchor) -> type& {           \
    return Expression::create_authored<type>(                               \
        domain, anchor, [&](auto source) -> type {                          \
          return type(domain, left, right, source);                         \
        });                                                                 \
  }                                                                         \
  auto Tetrodotoxin::Library::Language::Operations::type::create_synthetic( \
      Perimortem::Memory::Allocator::Arena& domain, Model::Pack& left,      \
      Model::Pack& right) -> type& {                                        \
    return Expression::create_synthetic<type>(                              \
        domain, [&](auto source) -> type {                                  \
          return type(domain, left, right, source);                         \
        });                                                                 \
  }                                                                         \
  Tetrodotoxin::Library::Language::Operations::type::type(                  \
      Perimortem::Memory::Allocator::Arena& domain, Model::Pack& left,      \
      Model::Pack& right,                                                   \
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)                \
      : Operation(                                                          \
            domain,                                                         \
            Perimortem::Core::Static::Vector<                               \
                Tetrodotoxin::Source::PackReference<Model::Pack>, 2>{{left, right}},  \
            anchor) {}

#define TTX_UNARY_OP(type)                                                   \
  auto Tetrodotoxin::Library::Language::Operations::type::create_authored(   \
      Perimortem::Memory::Allocator::Arena& domain, Model::Pack& operand,    \
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> type& {                                \
    return Expression::create_authored<type>(                                \
        domain, anchor,                                                      \
        [&](auto source) -> type { return type(domain, operand, source); }); \
  }                                                                          \
  auto Tetrodotoxin::Library::Language::Operations::type::create_synthetic(  \
      Perimortem::Memory::Allocator::Arena& domain, Model::Pack& operand)    \
      -> type& {                                                             \
    return Expression::create_synthetic<type>(                               \
        domain,                                                              \
        [&](auto source) -> type { return type(domain, operand, source); }); \
  }                                                                          \
  Tetrodotoxin::Library::Language::Operations::type::type(                   \
      Perimortem::Memory::Allocator::Arena& domain, Model::Pack& operand,    \
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)                 \
      : Operation(                                                           \
            domain,                                                          \
            Perimortem::Core::Static::Vector<                                \
                Tetrodotoxin::Source::PackReference<Model::Pack>, 1>{{operand}},       \
            anchor) {}
