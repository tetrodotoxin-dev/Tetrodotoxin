// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/type_reference.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/parser/layout.hpp"
#include "tetrodotoxin/library/interpreter/literal.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::TypeReference::parse(const Abstract& context, Cursor& cursor)
    -> Core::Option<Language::TypeReference> {
  auto& domain = cursor.get_arena();
  auto route = parse_route(cursor);
  BAIL_IF(!route);
  if (!cursor.matches(Code::Type::BracketStart)) {
    return *route;
  }

  Memory::Managed::Vector<Language::TypeReference::Argument> arguments(domain);
  auto closing = Tetrodotoxin::Language::Parser::Layout::parse_entries(
      cursor, Code::Type::BracketStart, Code::Type::BracketEnd,
      [&](Cursor& entry, Count) -> Bool {
        if (entry.matches(Code::Type::Type)) {
          auto nested = parse(context, entry);
          BAIL_IF(!nested);

          // The nested route lives in the source Arena beside the argument
          // shape. Retaining that value gives later Generic resolution the
          // exact authored edge without keeping this parser alive.
          const Language::TypeReference& retained =
              domain.construct<Language::TypeReference>(*nested);
          arguments.insert(Language::TypeReference::Argument(retained));
          return True;
        }

        switch (entry.current().get_code().get_type()) {
        case Code::Type::Numeric:
        case Code::Type::Hex:
        case Code::Type::Float:
        case Code::Type::String:
        case Code::Type::Bytes:
        case Code::Type::Embedded:
        case Code::Type::True:
        case Code::Type::False:
          break;
        default:
          entry.create_token_error(
              "Library Generic Layout entries require a Type reference or "
              "literal."_view);
          return False;
        }

        auto literal = Literal::parse(context, entry);
        BAIL_IF(!literal);
        arguments.insert(Language::TypeReference::Argument(*literal));
        return True;
      });
  BAIL_IF(!closing);

  return Language::TypeReference::create_authored(
      cursor.get_source_text().slice(
          route->get_anchor().get_span().get_offset(),
          route->get_anchor().get_span().get_size()),
      Anchor::create(
          route->get_anchor().get_token(),
          Span(route->get_anchor().get_token(), *closing)),
      route->get_anchor().get_span().get_end(), arguments.get_view());
}

auto Interpreter::TypeReference::parse_route(Cursor& cursor)
    -> Core::Option<Language::TypeReference> {
  Token first = cursor.require(
      Code::Type::Type, "Library Type reference requires one Type name."_view);
  BAIL_IF(!first);

  Token last = first;
  while (cursor.matches(Code::Type::TypeAccessOp)) {
    Token separator = cursor.current();
    Count previous_end = Count(last.get_offset()) + Count(last.get_size());
    if (separator.get_offset() != previous_end) {
      cursor.create_expression_error(
          Span(first, separator),
          "Library Type references cannot contain whitespace around `::`."_view);
      return {};
    }

    cursor.consume();
    Token segment = cursor.require(
        Code::Type::Type,
        "Library Type reference requires a Type after `::`."_view);
    BAIL_IF(!segment);
    Count separator_end =
        Count(separator.get_offset()) + Count(separator.get_size());
    if (segment.get_offset() != separator_end) {
      cursor.create_expression_error(
          Span(first, segment),
          "Library Type references cannot contain whitespace around `::`."_view);
      return {};
    }
    last = segment;
  }

  Count start = first.get_offset();
  Count end = Count(last.get_offset()) + Count(last.get_size());
  return Language::TypeReference::create_authored(
      cursor.get_source_text().slice(start, end - start),
      Anchor::create(first, Span(first, last)), last);
}
