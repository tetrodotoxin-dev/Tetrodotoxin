// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/model/memory.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// Match owns one authored value selection. Constant cases compare one folded
// scalar domain. An Option uses one branch local value case for presence and
// the final discard case for absence. A selected body never falls through.
class Match : public Tetrodotoxin::Source::Abstract {
 public:
  enum class CaseKind : U8 {
    Constant,
    Value,
  };

  // Pattern lends the one branch local payload together with the exact context
  // that exposes it. The pair belongs to Match construction and never becomes
  // a general scope or declaration model.
  class Pattern {
   public:
    constexpr Pattern(
        Tetrodotoxin::Source::Abstract& context,
        Model::Memory& payload)
        : context(context), payload(payload) {}

    constexpr auto get_context() const -> Tetrodotoxin::Source::Abstract& {
      return context.get();
    }

    constexpr auto get_payload() const -> Model::Memory& {
      return payload.get();
    }

   private:
    Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract> context;
    Tetrodotoxin::Source::Reference<Model::Memory> payload;
  };

 private:
  struct Case {
    CaseKind kind;
    Perimortem::Core::Option<Tetrodotoxin::Source::PackReference<Model::Pack>> value;
    Tetrodotoxin::Source::Reference<Block> body;
    Perimortem::Core::Option<Tetrodotoxin::Source::Reference<Model::Memory>>
        payload;
    Tetrodotoxin::Source::Lexical::Anchor anchor;
    Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Constant>> constant;
  };

 public:
  TTX_CONTRACT(Match, Tetrodotoxin::Source::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& input,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Match&;

  static auto create_pattern(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Abstract& parent,
      Perimortem::Core::View::Bytes name) -> Pattern;

  auto retain_value_case(
      Model::Pack& value,
      Block& body,
      Model::Memory& payload,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> void;

  auto retain_constant_case(
      Model::Pack& value,
      Block& body,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> void;

  auto complete_default(Block& body) -> Bool;

  auto complete_anchor(Tetrodotoxin::Source::Lexical::Anchor selected) -> void;

  Match(const Match&) = delete;
  Match(Match&&) = delete;
  auto operator=(const Match&) -> Match& = delete;
  auto operator=(Match&&) -> Match& = delete;

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      const Model::Type& access_scope) -> Bool;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void;

  auto reaches_next_statement() const -> Bool;

  TTX_NAME("Match"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_input() const -> const Model::Pack& { return input.get(); }

  constexpr auto get_case_count() const -> Count { return cases.get_size(); }

  auto get_case_constant(Count index) const
      -> Perimortem::Core::Option<const Constant&>;

  auto get_case_kind(Count index) const -> Perimortem::Core::Option<CaseKind>;

  auto get_case_payload(Count index) const
      -> Perimortem::Core::Option<const Model::Memory&>;

  auto get_case_body(Count index) const
      -> Perimortem::Core::Option<const Block&>;

  auto get_case_anchor(Count index) const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor>;

  constexpr auto get_default() const -> Perimortem::Core::Option<const Block&> {
    return default_body.visit(
        []() -> Perimortem::Core::Option<const Block&> { return {}; },
        [](const Tetrodotoxin::Source::Reference<Block>& selected)
            -> Perimortem::Core::Option<const Block&> {
          return selected.get();
        });
  }

  constexpr auto has_complete_coverage() const -> Bool {
    return complete_coverage;
  }

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

 private:
  Match(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& input,
      Tetrodotoxin::Source::Lexical::Anchor anchor)
      : input(input), cases(domain), anchor(anchor) {}

  Tetrodotoxin::Source::PackReference<Model::Pack> input;
  Perimortem::Memory::Managed::Vector<Case> cases;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<Block>> default_body;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
  Bool complete_coverage = False;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
