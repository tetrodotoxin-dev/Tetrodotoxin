// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Library::Interpreter {

// A parser can reject a semantic form after consuming its complete source
// boundary. Keeping that outcome distinct from incomplete grammar lets the
// enclosing owner continue at the next declaration without skipping it.
enum class ParseState : U8 {
  Accepted,
  Rejected,
  Incomplete,
};

// Parsed keeps one real semantic identity together with the outcome of the
// source form that produced it. Recovery can retain the identity for editor
// queries while completion still rejects the malformed source transaction.
template <typename semantic_type>
class Parsed {
 public:
  constexpr Parsed(semantic_type& semantic, ParseState state)
      : semantic(semantic), state(state) {}

  constexpr auto get_semantic() const -> semantic_type& {
    return semantic.get();
  }

  constexpr auto get_state() const -> ParseState { return state; }

  constexpr auto is_accepted() const -> Bool {
    return state == ParseState::Accepted;
  }

  constexpr auto needs_recovery() const -> Bool {
    return state == ParseState::Incomplete;
  }

 private:
  Tetrodotoxin::Source::Reference<semantic_type> semantic;
  ParseState state;
};

}  // namespace Tetrodotoxin::Library::Interpreter
