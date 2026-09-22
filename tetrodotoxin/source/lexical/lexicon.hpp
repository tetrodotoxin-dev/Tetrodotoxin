// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/source/lexical/code.hpp"

namespace Tetrodotoxin::Source::Lexical {

// Lexicon owns the source spelling rules in one concrete Lexer contract. Code
// owns the prescribed semantic grouping while Lexicon owns the fixed and
// variable spellings that produce those groupings. It can also validate a
// complete spelling composed from one Code shape and explicit separator Codes.
// Codes whose spelling comes from an authored token payload return an empty
// view.
class Lexicon {
 public:
  Lexicon() = delete;

  // Accepts one continuation byte after the uppercase opening byte of a Type
  // name.
  static constexpr auto is_type(U8 byte) -> Bool {
    return (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
           (byte >= '0' && byte <= '9') || byte == '_';
  }

  // Accepts one byte from the lowercase identifier character set.
  static constexpr auto is_identifier(U8 byte) -> Bool {
    return (byte >= 'a' && byte <= 'z') || (byte >= '0' && byte <= '9') ||
           byte == '_';
  }

  // Accepts one byte from the decimal scanner character set.
  static constexpr auto is_numeric(U8 byte) -> Bool {
    return (byte >= '0' && byte <= '9') || byte == '.';
  }

  // Accepts one byte from the hexadecimal digit character set.
  static constexpr auto is_hex(U8 byte) -> Bool {
    return (byte >= '0' && byte <= '9') || (byte >= 'a' && byte <= 'f') ||
           (byte >= 'A' && byte <= 'F');
  }

  // Accepts one byte ignored between authored lexical values.
  static constexpr auto is_whitespace(U8 byte) -> Bool {
    return byte == ' ' || byte == '\n' || byte == '\r' || byte == '\t';
  }

  // Converts one byte already proven by is_hex into its numeric value.
  static constexpr auto get_hex_value(U8 byte) -> U8 {
    if (byte <= '9') {
      return byte - '0';
    }
    if (byte <= 'F') {
      return byte - 'A' + 10;
    }
    return byte - 'a' + 10;
  }

  // Proves that one complete authored byte span has the requested Code shape.
  // When separators are supplied the span contains one or more values joined
  // by any listed separator Code.
  static auto validate(
      Code::Type type,
      Perimortem::Core::View::Bytes value,
      Perimortem::Core::View::Vector<Code::Type> separators = {}) -> Bool;

  static constexpr auto get_spelling(Code::Type type)
      -> Perimortem::Core::View::Bytes {
    switch (type) {
    // Modifiers
    case Code::Type::Public:
      return "public"_view;
    case Code::Type::Private:
      return "private"_view;
    case Code::Type::Expose:
      return "expose"_view;
    case Code::Type::State:
      return "state"_view;
    case Code::Type::Const:
      return "const"_view;

    // Definition keywords
    case Code::Type::Alias:
      return "alias"_view;
    case Code::Type::Namespace:
      return "namespace"_view;
    case Code::Type::Enum:
      return "enum"_view;
    case Code::Type::Struct:
      return "struct"_view;
    case Code::Type::Object:
      return "object"_view;
    case Code::Type::Interface:
      return "interface"_view;
    case Code::Type::Implementation:
      return "implementation"_view;

    // Statement and import keywords
    case Code::Type::Using:
      return "using"_view;
    case Code::Type::New:
      return "new"_view;
    case Code::Type::If:
      return "if"_view;
    case Code::Type::In:
      return "in"_view;
    case Code::Type::For:
      return "for"_view;
    case Code::Type::Break:
      return "break"_view;
    case Code::Type::Continue:
      return "continue"_view;
    case Code::Type::Match:
      return "match"_view;
    case Code::Type::Case:
      return "case"_view;
    case Code::Type::Else:
      return "else"_view;
    case Code::Type::While:
      return "while"_view;
    case Code::Type::Return:
      return "return"_view;
    case Code::Type::Emit:
      return "emit"_view;
    case Code::Type::Source:
      return "source"_view;
    case Code::Type::Package:
      return "package"_view;
    case Code::Type::Dialect:
      return "dialect"_view;
    case Code::Type::Func:
      return "func"_view;

    // Binary operators
    case Code::Type::AddOp:
      return "+"_view;
    case Code::Type::SubOp:
      return "-"_view;
    case Code::Type::MulOp:
      return "*"_view;
    case Code::Type::DivOp:
      return "/"_view;
    case Code::Type::ModOp:
      return "%"_view;
    case Code::Type::CmpOp:
      return "=="_view;
    case Code::Type::NotEqOp:
      return "!="_view;
    case Code::Type::LessOp:
      return "<"_view;
    case Code::Type::GreaterOp:
      return ">"_view;
    case Code::Type::LessEqOp:
      return "<="_view;
    case Code::Type::GreaterEqOp:
      return ">="_view;
    case Code::Type::And:
      return "and"_view;
    case Code::Type::Or:
      return "or"_view;
    case Code::Type::AndOp:
      return "&"_view;
    case Code::Type::OrOp:
      return "|"_view;

    // Assignment operators
    case Code::Type::Assign:
      return "="_view;
    case Code::Type::AddAssign:
      return "+="_view;
    case Code::Type::SubAssign:
      return "-="_view;

    // Address, packing, and parser operators
    case Code::Type::ScopeStart:
      return "{"_view;
    case Code::Type::ScopeEnd:
      return "}"_view;
    case Code::Type::PackingStart:
      return "("_view;
    case Code::Type::PackingEnd:
      return ")"_view;
    case Code::Type::BracketStart:
      return "["_view;
    case Code::Type::BracketEnd:
      return "]"_view;
    case Code::Type::Define:
      return ":"_view;
    case Code::Type::TypeAccessOp:
      return "::"_view;
    case Code::Type::EndStatement:
      return ";"_view;
    case Code::Type::CallOp:
      return "->"_view;
    case Code::Type::AddressOp:
      return "."_view;
    case Code::Type::SwizzleOp:
      return ".["_view;
    case Code::Type::ValueAccessOp:
      return ":["_view;
    case Code::Type::PackingOp:
      return ","_view;
    case Code::Type::NotOp:
      return "!"_view;
    case Code::Type::QuestionOp:
      return "?"_view;
    case Code::Type::RangeOp:
      return "..."_view;
    case Code::Type::Discard:
      return "_"_view;

    // Other fixed keywords and markers
    case Code::Type::Self:
      return "self"_view;
    case Code::Type::True:
      return "true"_view;
    case Code::Type::False:
      return "false"_view;

    // Literal markers
    case Code::Type::Hex:
      return "0x"_view;
    case Code::Type::Bytes:
      return "0x["_view;
    case Code::Type::Embedded:
      return "$["_view;

    // Other fixed markers
    case Code::Type::Attribute:
      return "@"_view;
    case Code::Type::Comment:
      return "//"_view;
    case Code::Type::RawComment:
      return "///"_view;
    case Code::Type::String:
      return "\""_view;

    default:
      return ""_view;
    }
  }

  // Maps names promoted out of the Addressable source space onto their
  // prescribed Codes. A name that is not reserved keeps the caller supplied
  // Addressable grouping.
  static constexpr auto get_keyword(
      Perimortem::Core::View::Bytes spelling,
      Code::Type fallback) -> Code::Type {
    using Entry =
        Perimortem::Utility::Pair<Perimortem::Core::View::Bytes, Code::Type>;

    static constexpr Perimortem::Core::Static::Vector<Entry, 34> keywords = {{
      Entry{get_spelling(Code::Type::And), Code::Type::And},
      {get_spelling(Code::Type::Or), Code::Type::Or},
      {get_spelling(Code::Type::If), Code::Type::If},
      {get_spelling(Code::Type::In), Code::Type::In},
      {get_spelling(Code::Type::For), Code::Type::For},
      {get_spelling(Code::Type::While), Code::Type::While},
      {get_spelling(Code::Type::Case), Code::Type::Case},
      {get_spelling(Code::Type::Match), Code::Type::Match},
      {get_spelling(Code::Type::Break), Code::Type::Break},
      {get_spelling(Code::Type::Continue), Code::Type::Continue},
      {get_spelling(Code::Type::Else), Code::Type::Else},
      {get_spelling(Code::Type::Func), Code::Type::Func},
      {get_spelling(Code::Type::Self), Code::Type::Self},
      {get_spelling(Code::Type::True), Code::Type::True},
      {get_spelling(Code::Type::False), Code::Type::False},
      {get_spelling(Code::Type::Return), Code::Type::Return},
      {get_spelling(Code::Type::Emit), Code::Type::Emit},
      {get_spelling(Code::Type::Source), Code::Type::Source},
      {get_spelling(Code::Type::Package), Code::Type::Package},
      {get_spelling(Code::Type::Dialect), Code::Type::Dialect},
      {get_spelling(Code::Type::Alias), Code::Type::Alias},
      {get_spelling(Code::Type::Namespace), Code::Type::Namespace},
      {get_spelling(Code::Type::Enum), Code::Type::Enum},
      {get_spelling(Code::Type::Struct), Code::Type::Struct},
      {get_spelling(Code::Type::Object), Code::Type::Object},
      {get_spelling(Code::Type::Interface), Code::Type::Interface},
      {get_spelling(Code::Type::Implementation), Code::Type::Implementation},
      {get_spelling(Code::Type::Using), Code::Type::Using},
      {get_spelling(Code::Type::New), Code::Type::New},
      {get_spelling(Code::Type::Public), Code::Type::Public},
      {get_spelling(Code::Type::Private), Code::Type::Private},
      {get_spelling(Code::Type::Expose), Code::Type::Expose},
      {get_spelling(Code::Type::State), Code::Type::State},
      {get_spelling(Code::Type::Const), Code::Type::Const},
    }};
    return Perimortem::Utility::Table<Code::Type, keywords>::find_or_default(
        spelling, fallback);
  }
};

}  // namespace Tetrodotoxin::Source::Lexical
