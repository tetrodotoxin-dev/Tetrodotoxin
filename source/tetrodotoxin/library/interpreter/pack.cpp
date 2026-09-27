// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/pack.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/interpreter/expression.hpp"
#include "tetrodotoxin/source/reference.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Pack::parse(
    const Abstract& context,
    Cursor& cursor,
    Bool force_parentheses) -> Core::Option<Language::Model::Pack&> {
  Memory::Allocator::Arena& domain = cursor.get_arena();
  // An ordinary Pack consumer needs the complete value flow expression so a
  // parenthesized Pack may itself receive postfix access. Expression calls the
  // forced path below only while parsing that parenthesized primary, which
  // breaks the mutual grammar recursion at one explicit delimiter boundary.
  if (!force_parentheses) {
    return Interpreter::Expression::parse(context, cursor);
  }

  if (!cursor.matches(Code::Type::PackingStart)) {
    cursor.create_token_error(
        "Library Pack requires one opening parenthesis."_view);
    return {};
  }

  Token opening = cursor.consume();
  Memory::Managed::Vector<Tetrodotoxin::Source::PackReference<Language::Model::Pack>>
      entries(domain);
  Memory::Managed::Vector<Core::View::Bytes> names(domain);
  Core::Option<Bool> named;

  while (!cursor.matches(Code::Type::PackingEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_expression_error(
          Span(opening, cursor.current()),
          "Library Pack requires its closing parenthesis."_view);
      return {};
    }

    Core::Option<Token> name_token;
    if (cursor.matches(Code::Type::AddressOp)) {
      cursor.consume();
      Token name = cursor.require(
          Code::Type::Addressable,
          "Named Library Pack entries require a name after `.`."_view);
      BAIL_IF(!name);
      BAIL_IF(!cursor.require(
          Code::Type::Assign,
          "Named Library Pack entries require `=` before their value."_view));

      Core::View::Bytes spelling = name.caculate_text(cursor.get_source_text());
      if (names.get_view().contains([&](Core::View::Bytes retained) {
            return retained == spelling;
          })) {
        cursor.create_token_error(
            name, "Duplicate name in one Library Pack."_view);
        return {};
      }
      name_token = name;
      names.insert(spelling);
    }

    Bool entry_named = bool(name_token);
    if (!named) {
      named = entry_named;
    } else if (*named != entry_named) {
      cursor.create_token_error(
          "Positional and named entries cannot share one Library Pack."_view);
      return {};
    }

    // Each delimited slot consumes one complete Expression grammar. That
    // grammar may itself start with another parenthesized Pack, so nested
    // groups retain fluid flow while postfix access still binds to the whole
    // inner Pack before this separator is considered.
    auto entry = Interpreter::Expression::parse(context, cursor);
    BAIL_IF(!entry);
    entries.insert(*entry);

    if (cursor.matches(Code::Type::PackingEnd)) {
      break;
    }
    BAIL_IF(!cursor.require(
        Code::Type::PackingOp,
        "Library Pack entries require `,` or the closing parenthesis."_view));
    if (cursor.matches(Code::Type::PackingEnd)) {
      break;
    }
  }

  Token closing = cursor.require(
      Code::Type::PackingEnd,
      "Library Pack requires its closing parenthesis."_view);
  BAIL_IF(!closing);

  if (entries.get_size() == 1 && (!named || !*named)) {
    Language::Model::Pack& selected = entries.at(0).get();
    return selected;
  }

  Core::Option<Anchor> anchor(Anchor::create(opening, Span(opening, closing)));
  Language::Model::Pack& group = Language::Model::Pack::create_group(
      domain, entries.get_view(), names.get_view(), anchor);
  return group;
}
