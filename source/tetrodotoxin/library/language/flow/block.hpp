// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/flow/scope.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/statement.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// Block is one authored Function body or nested lexical scope. It retains
// Statement memberships carry no identity while every entry keeps its exact
// graph object and Documentation backed by source. Its lexical parent,
// owning Function, and host Type remain independent facts.
// `{` admits an empty or multi Statement body, while `:` admits exactly one
// Statement without constructing a different semantic owner.
// This is not a lowered basic block and owns no predecessor arguments, result
// Layout, SSA edges, or target control flow.
class Block : public Scope {
 public:
  TTX_CONTRACT(Block, Scope);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Model::Callable& function,
      const Model::Type& access_scope,
      Perimortem::Core::Option<Tetrodotoxin::Source::Reference<
          const Tetrodotoxin::Source::Abstract>> enclosing_loop = {}) -> Block&;

  auto retain_authored_statement(Statement statement) -> void;

  auto complete_authored(Tetrodotoxin::Source::Lexical::Anchor selected) -> void;

  Block(const Block&) = delete;
  Block(Block&&) = delete;
  auto operator=(const Block&) -> Block& = delete;
  auto operator=(Block&&) -> Block& = delete;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void;

  auto reaches_next_statement() const -> Bool;

  TTX_NAME("Block"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  // An authored lookup uses the querying Token's position instead of the
  // transient linking prefix. A retained query can therefore revisit the same
  // lexical Block after linking advances without admitting a declaration that
  // appears later in source.
  auto resolve_authored_context(
      Perimortem::Core::View::Bytes route,
      Count offset) const -> const Tetrodotoxin::Source::Abstract&;

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

  constexpr auto get_statements() const
      -> Perimortem::Core::View::Vector<Statement> {
    return statements;
  }

  constexpr auto get_enclosing_loop() const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> override {
    return enclosing_loop.visit(
        []() -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> {
          return {};
        },
        [](const Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>& loop)
            -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> {
          return loop.get();
        });
  }

  constexpr auto get_function_results() const
      -> const Tetrodotoxin::Source::Layout& override {
    return function.get_results();
  }

  constexpr auto get_access_scope() const -> const Model::Type& override {
    return access_scope;
  }

 private:
  Block(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Model::Callable& function,
      const Model::Type& access_scope,
      Perimortem::Core::Option<
          Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>> enclosing_loop)
      : lexical_context(lexical_context),
        function(function),
        access_scope(access_scope),
        statements(domain),
        enclosing_loop(enclosing_loop) {}

  const Tetrodotoxin::Source::Abstract& lexical_context;
  Model::Callable& function;
  const Model::Type& access_scope;
  Perimortem::Memory::Managed::Vector<Statement> statements;
  Perimortem::Core::Option<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>>
      enclosing_loop;
  // Linking advances this prefix before each Statement so name lookup observes
  // only declarations whose source position precedes the active entry. It is
  // transient phase state, not another declaration inventory.
  Count linked_prefix_size = 0;
  Tetrodotoxin::Source::Lexical::Anchor anchor = Tetrodotoxin::Source::Lexical::Anchor::create({});
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
