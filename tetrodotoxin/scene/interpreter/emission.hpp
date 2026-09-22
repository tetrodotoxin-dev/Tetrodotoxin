// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/interpreter/execution/statement_parser.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"

namespace Tetrodotoxin::Scene::Interpreter {

// Emission composes the one Scene statement family into Library Block parsing.
// The same parser instance follows nested Library control flow, so `emit` keeps
// its Scene meaning without a second executable grammar.
class Emission final
    : public Tetrodotoxin::Library::Interpreter::Execution::StatementParser {
 public:
  constexpr Emission(Language::Monograph& monograph) : monograph(monograph) {}

  auto matches(const Tetrodotoxin::Source::Lexical::Cursor& cursor) const -> Bool override;

  auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Library::Language::Flow::Block& block,
      Tetrodotoxin::Library::Language::Model::Callable& function,
      const Tetrodotoxin::Library::Language::Model::Type& access_scope,
      const Tetrodotoxin::Source::Documentation& documentation) const
      -> Perimortem::Core::Option<
          Tetrodotoxin::Library::Language::Statement> override;

 private:
  Language::Monograph& monograph;
};

}  // namespace Tetrodotoxin::Scene::Interpreter
