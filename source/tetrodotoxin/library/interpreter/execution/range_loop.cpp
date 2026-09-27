// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/execution/range_loop.hpp"

#include "tetrodotoxin/language/parser/layout.hpp"
#include "tetrodotoxin/library/interpreter/execution/block.hpp"
#include "tetrodotoxin/library/interpreter/expression.hpp"
#include "tetrodotoxin/library/interpreter/type_reference.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Execution::RangeLoop::parse(
    Cursor& cursor,
    Language::Flow::Block& lexical_context,
    Language::Model::Callable& function,
    const Language::Model::Type& access_scope,
    Option<const StatementParser&> extension)
    -> Option<Language::Flow::RangeLoop&> {
  Token opening = cursor.require(
      Code::Type::For, "Library for loops require the `for` keyword."_view);
  BAIL_IF(!opening);
  if (!cursor.matches(Code::Type::BracketStart)) {
    cursor.create_token_error(
        "Library for loop bindings require one bracketed named Layout."_view);
    return {};
  }

  Managed::Vector<Language::Flow::RangeLoop::AuthoredBinding> bindings(
      cursor.get_arena());
  auto binding_end = Tetrodotoxin::Language::Parser::Layout::parse(
      cursor, False,
      [&](Cursor& entry, Count, Option<Token> selected_name,
          View::Vector<Tetrodotoxin::Language::Attribute> attributes) -> Bool {
        if (!attributes.is_empty()) {
          entry.create_expression_error(
              attributes.get_data()[0].get_anchor(),
              "Library for loop bindings do not carry declaration Attributes."_view);
          return False;
        }
        if (!selected_name ||
            selected_name->get_code() != Code::Type::Addressable) {
          entry.create_token_error(
              "A Library for loop binding requires `.name : Type`."_view);
          return False;
        }
        View::Bytes name =
            selected_name->caculate_text(cursor.get_source_text());
        if (bindings.get_view().contains(
                [&](const auto& existing) { return existing.name == name; })) {
          entry.create_token_error(
              *selected_name,
              "A Library for loop binding name must be unique."_view);
          return False;
        }
        auto selected_type =
            Interpreter::TypeReference::parse(lexical_context, entry);
        BAIL_IF(!selected_type);
        bindings.insert({*selected_name, name, *selected_type});
        return True;
      });
  BAIL_IF(!binding_end);
  if (bindings.is_empty()) {
    cursor.create_expression_error(
        Span(opening, *binding_end),
        "A Library for loop requires at least one named binding."_view,
        "Use `[.name : Type]` before the `in` keyword."_view);
    return {};
  }
  BAIL_IF(!cursor.require(
      Code::Type::In,
      "Library for loop bindings require the `in` keyword."_view));
  auto input = Interpreter::Expression::parse(lexical_context, cursor);
  BAIL_IF(!input);

  Language::Flow::RangeLoop& loop = Language::Flow::RangeLoop::create_authored(
      cursor.get_arena(), lexical_context, bindings.get_view(), *input,
      Anchor::create(opening, Span(opening, cursor.peek(-1))));
  auto body = Block::parse(
      cursor, loop, function, access_scope, Reference<const Abstract>(loop),
      extension);
  BAIL_IF(!body);
  BAIL_IF(!loop.complete_body(
      *body, Anchor::create(opening, Span(opening, cursor.peek(-1)))));
  return loop;
}
