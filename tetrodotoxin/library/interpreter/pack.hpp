// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Interpreter {

// Pack is the stateless lexical entry point for Library value flow. It owns
// parentheses, separators, source order, explicit slot names, and their shared
// diagnostics. A bare or single value positional form returns the exact child
// Pack. Empty and named forms, along with forms containing several values,
// construct one authored group over those real child identities.
class Pack {
 public:
  Pack() = delete;

  static auto parse(
      const Tetrodotoxin::Source::Abstract& context,
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Bool force_parentheses = False)
      -> Perimortem::Core::Option<Language::Model::Pack&>;
};

}  // namespace Tetrodotoxin::Library::Interpreter
