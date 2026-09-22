// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/interpreter/parsed.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter {

// Member selects the concrete parser named by one Definition qualifier. It
// retains no state and returns the real semantic object constructed by that
// owner. The receiving Composite alone decides whether to retain it.
class Member {
 public:
  // Result carries the exact declaration identity together with the category
  // selected by its qualifier. It exists only for this parser call and never
  // becomes a second declaration record in the retained graph.
  class Result {
   public:
    constexpr Result(
        Tetrodotoxin::Source::Abstract& semantic,
        Language::Types::Composite::Category category,
        ParseState state)
        : semantic(semantic), category(category), state(state) {}

    constexpr auto get_semantic() const -> Tetrodotoxin::Source::Abstract& {
      return semantic.get();
    }

    constexpr auto get_category() const
        -> Language::Types::Composite::Category {
      return category;
    }

    constexpr auto is_accepted() const -> Bool {
      return state == ParseState::Accepted;
    }

    constexpr auto needs_recovery() const -> Bool {
      return state == ParseState::Incomplete;
    }

   private:
    Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract> semantic;
    Language::Types::Composite::Category category;
    ParseState state;
  };

  Member() = delete;

  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Result>;
};

}  // namespace Tetrodotoxin::Library::Interpreter
