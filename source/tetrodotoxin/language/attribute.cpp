// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/attribute.hpp"

#include "perimortem/core/option.hpp"
#include "perimortem/core/reader/textual.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/source/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

static auto parse_string(Cursor& cursor) -> Option<Language::Attribute::Value> {
  BAIL_IF(!cursor.matches(Code::Type::String));

  Token token = cursor.consume();
  View::Bytes text = token.caculate_text(cursor.get_source_text());
  if (!Lexicon::validate(Code::Type::String, text)) {
    cursor.create_token_error(
        token, "Attribute string values require complete quoted text."_view);
    return {};
  }

  View::Bytes payload = text.slice(1, text.get_size() - 2);
  Count decoded_size = payload.get_size();
  // Escapes belong to source spelling. Count before allocation so the retained
  // Attribute receives one exact decoded Bytes value.
  for (Count i = 0; i < payload.get_size(); i++) {
    if (payload[i] == '\\') {
      decoded_size--;
      i++;
    }
  }

  auto decoded = cursor.get_arena().allocate(decoded_size);
  Count output = 0;
  for (Count i = 0; i < payload.get_size(); i++) {
    if (payload[i] == '\\') {
      i++;
    }
    decoded.get_data()[output] = payload[i];
    output++;
  }

  return Language::Attribute::Value(
      View::Bytes(decoded.get_data(), decoded.get_size()));
}

static auto parse_number(Cursor& cursor) -> Option<Language::Attribute::Value> {
  Bool negative = cursor.matches(Code::Type::SubOp);
  Token first = cursor.current();
  Token number = negative ? cursor.peek(1) : first;
  Count width = negative ? 2 : 1;
  BAIL_IF(
      number.get_code() != Code::Type::Numeric &&
      number.get_code() != Code::Type::Hex &&
      number.get_code() != Code::Type::Float);

  Option<Language::Attribute::Value> value;
  if (number.get_code() == Code::Type::Float) {
    View::Bytes text =
        Span(first, number).caculate_text(cursor.get_source_text());
    Reader::Textual reader(text);
    R64 parsed = reader.read_r64();
    if (reader.is_valid() && reader.get_location() == reader.get_size()) {
      value = Language::Attribute::Value(parsed);
    }
  } else if (negative && number.get_code() == Code::Type::Numeric) {
    View::Bytes text =
        Span(first, number).caculate_text(cursor.get_source_text());
    Reader::Textual reader(text);
    S64 parsed = reader.read_signed();
    if (reader.is_valid() && reader.get_location() == reader.get_size()) {
      value = Language::Attribute::Value(parsed);
    }
  } else {
    View::Bytes text = number.caculate_text(cursor.get_source_text());
    Reader::Textual reader(
        number.get_code() == Code::Type::Hex ? text.slice(2) : text);
    U64 parsed = reader.read_unsigned(
        number.get_code() == Code::Type::Hex ? Count(16) : Count(10));
    if (reader.is_valid() && reader.get_location() == reader.get_size()) {
      if (!negative) {
        value = Language::Attribute::Value(parsed);
      } else if (parsed <= (U64(1) << 63)) {
        S64 signed_value = parsed == (U64(1) << 63)
                               ? S64(-9223372036854775807LL - 1)
                               : -S64(parsed);
        value = Language::Attribute::Value(signed_value);
      }
    }
  }

  if (value) {
    for (Count i = 0; i < width; i++) {
      cursor.consume();
    }
  }
  return value;
}

static auto parse_value(Cursor& cursor) -> Option<Language::Attribute::Value> {
  if (cursor.matches(Code::Type::String)) {
    return parse_string(cursor);
  }
  if (cursor.matches(Code::Type::True) || cursor.matches(Code::Type::False)) {
    return Language::Attribute::Value(
        cursor.consume().get_code() == Code::Type::True ? True : False);
  }

  return parse_number(cursor);
}

auto Language::Attribute::parse(Cursor& cursor) -> View::Vector<Attribute> {
  if (!cursor.matches(Code::Type::Attribute)) {
    return {};
  }

  Managed::Vector<Attribute> attributes(cursor.get_arena());
  while (cursor.matches(Code::Type::Attribute)) {
    Token key_token = cursor.consume();
    View::Bytes key = key_token.caculate_text(cursor.get_source_text());
    Bool valid_key = !key.is_empty();
    for (Count i = 0; i < key.get_size(); i++) {
      valid_key &= Lexicon::is_identifier(key[i]);
    }
    if (!valid_key) {
      cursor.create_token_error(
          key_token, "Attributes require one authored key."_view);
      return {};
    }

    Token closing = key_token;
    Attribute::Value value;
    if (cursor.matches(Code::Type::PackingStart)) {
      cursor.consume();
      auto parsed_value = parse_value(cursor);
      if (!parsed_value) {
        cursor.create_token_error(
            "Attribute values require one scalar literal."_view);
        return {};
      }
      value = *parsed_value;

      closing = cursor.require(
          Code::Type::PackingEnd,
          "Attribute values require one closing `)`."_view);
      BAIL_IF(!closing);
    }

    attributes.insert(Attribute(
        key, value, Anchor::create(key_token, Span(key_token, closing))));
  }

  return attributes.get_view();
}
