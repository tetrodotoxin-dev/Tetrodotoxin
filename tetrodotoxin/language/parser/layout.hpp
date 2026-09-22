// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/attribute.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Language::Parser {

// Layout owns the shared authored envelope for descriptor flow. It interprets
// optional brackets, separators, a trailing comma, Attributes, and `.name :`
// prefixes while the supplied consumer creates each concrete semantic slot.
// A Dialect remains free to choose its Type routes, receiver forms, and lasting
// Layout model.
class Layout {
 public:
  Layout() = delete;

  template <typename consume_type>
  static auto parse(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Bool accepts_self,
      consume_type&& consume) -> Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Token> {
    if (!cursor.matches(Tetrodotoxin::Source::Lexical::Code::Type::BracketStart)) {
      auto attributes = Tetrodotoxin::Language::Attribute::parse(cursor);
      BAIL_IF(!consume(
          cursor, Count(0), Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Token>(),
          attributes));
      return cursor.peek(-1);
    }

    Perimortem::Core::Option<Bool> named;
    Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes> names(
        cursor.get_arena());
    return parse_entries(
        cursor, Tetrodotoxin::Source::Lexical::Code::Type::BracketStart,
        Tetrodotoxin::Source::Lexical::Code::Type::BracketEnd,
        [&](Tetrodotoxin::Source::Lexical::Cursor& entry, Count index) -> Bool {
          auto attributes = Tetrodotoxin::Language::Attribute::parse(entry);
          Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Token> name;
          Bool entry_named = False;
          if (entry.matches(Tetrodotoxin::Source::Lexical::Code::Type::AddressOp)) {
            entry.consume();
            Tetrodotoxin::Source::Lexical::Token name_token = entry.require(
                Tetrodotoxin::Source::Lexical::Code::Type::Addressable,
                "Named Layout entries require a name after `.`."_view);
            BAIL_IF(!name_token);
            BAIL_IF(!retain_name(entry, name_token, names));
            BAIL_IF(!entry.require(
                Tetrodotoxin::Source::Lexical::Code::Type::Define,
                "Named Layout entries require `:` before their descriptor."_view));
            name = name_token;
            entry_named = True;
          } else if (
              accepts_self && entry.matches(Tetrodotoxin::Source::Lexical::Code::Type::Self)) {
            // Library uses `self` as one intrinsically named descriptor. The
            // concrete consumer decides whether its position and role are
            // meaningful.
            Tetrodotoxin::Source::Lexical::Token self = entry.current();
            BAIL_IF(!retain_name(entry, self, names));
            name = self;
            entry_named = True;
          }

          BAIL_IF(!require_shape(entry, named, entry_named));
          return consume(entry, index, name, attributes);
        });
  }

  template <typename consume_type>
  static auto parse_entries(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Source::Lexical::Code::Type opening,
      Tetrodotoxin::Source::Lexical::Code::Type closing,
      consume_type&& consume) -> Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Token> {
    Tetrodotoxin::Source::Lexical::Token opening_token =
        cursor.require(opening, "Layout requires its opening delimiter."_view);
    BAIL_IF(!opening_token);

    Count index = 0;
    while (!cursor.matches(closing)) {
      if (cursor.matches(Tetrodotoxin::Source::Lexical::Code::Type::Terminal)) {
        cursor.create_expression_error(
            Tetrodotoxin::Source::Lexical::Span(opening_token, cursor.current()),
            "Layout requires its closing delimiter."_view);
        return {};
      }

      BAIL_IF(!consume(cursor, index));
      index++;
      if (cursor.matches(closing)) {
        break;
      }

      BAIL_IF(!cursor.require(
          Tetrodotoxin::Source::Lexical::Code::Type::PackingOp,
          "Layout entries require `,` or the closing delimiter."_view));
      if (cursor.matches(closing)) {
        break;
      }
    }

    Tetrodotoxin::Source::Lexical::Token closing_token =
        cursor.require(closing, "Layout requires its closing delimiter."_view);
    BAIL_IF(!closing_token);
    return closing_token;
  }

 private:
  static auto require_shape(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Perimortem::Core::Option<Bool>& selected,
      Bool named) -> Bool;

  static auto retain_name(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      Tetrodotoxin::Source::Lexical::Token token,
      Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes>& names)
      -> Bool;
};

}  // namespace Tetrodotoxin::Language::Parser
