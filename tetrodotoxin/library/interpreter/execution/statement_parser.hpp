// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/statement.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::Library::Interpreter::Execution {

// StatementParser is the one explicit grammar composition edge carried by a
// Library Block. An embedding Dialect can recognize one additional statement
// family while every ordinary statement and nested control form remains owned
// by Library. One parser travels through the complete nested body, so there is
// no registry and no copied executable grammar.
class StatementParser {
 public:
  virtual constexpr ~StatementParser() = default;

  virtual auto matches(const Tetrodotoxin::Source::Lexical::Cursor& cursor) const -> Bool = 0;
  virtual auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Language::Flow::Block& block,
      Language::Model::Callable& function,
      const Language::Model::Type& access_scope,
      const Tetrodotoxin::Source::Documentation& documentation) const
      -> Perimortem::Core::Option<Language::Statement> = 0;
};

}  // namespace Tetrodotoxin::Library::Interpreter::Execution
