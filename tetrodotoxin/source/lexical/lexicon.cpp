// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;

static auto begins_with(View::Bytes value, View::Bytes prefix) -> Bool {
  return !prefix.is_empty() && prefix.get_size() <= value.get_size() &&
         value.slice(0, prefix.get_size()) == prefix;
}

// Type and Addressable values apply their distinct opening byte before the
// shared continuation character checks.
static auto validate_type(View::Bytes value) -> Bool {
  BAIL_IF(value.is_empty() || value[0] < 'A' || value[0] > 'Z');

  for (Count i = 1; i < value.get_size(); i++) {
    BAIL_IF(!Lexicon::is_type(value[i]));
  }

  return True;
}

static auto validate_addressable(View::Bytes value) -> Bool {
  BAIL_IF(value.is_empty() || value[0] < 'a' || value[0] > 'z');

  for (Count i = 1; i < value.get_size(); i++) {
    BAIL_IF(!Lexicon::is_identifier(value[i]));
  }

  return Lexicon::get_keyword(value, Code::Type::Addressable) ==
         Code::Type::Addressable;
}

// Numeric values contain only decimal digits while Float values contain one
// decimal point after an opening digit.
static auto validate_numeric(View::Bytes value) -> Bool {
  BAIL_IF(value.is_empty());

  for (Count i = 0; i < value.get_size(); i++) {
    BAIL_IF(!Lexicon::is_numeric(value[i]) || value[i] == '.');
  }

  return True;
}

static auto validate_float(View::Bytes value) -> Bool {
  BAIL_IF(value.is_empty() || value[0] < '0' || value[0] > '9');

  Count points = 0;
  for (Count i = 0; i < value.get_size(); i++) {
    BAIL_IF(!Lexicon::is_numeric(value[i]));

    if (value[i] == '.') {
      points++;
    }
  }

  return points == 1;
}

// Hex values include their fixed prefix and require at least one payload byte.
static auto validate_hex(View::Bytes value) -> Bool {
  View::Bytes prefix = Lexicon::get_spelling(Code::Type::Hex);
  BAIL_IF(!begins_with(value, prefix) || value.get_size() == prefix.get_size());

  for (Count i = prefix.get_size(); i < value.get_size(); i++) {
    BAIL_IF(!Lexicon::is_hex(value[i]));
  }

  return True;
}

// Range spellings require their closing byte to be the final authored byte.
// An earlier closing byte would have ended the Tokenizer range.
static auto validate_range(View::Bytes value, Code::Type type, U8 terminal)
    -> Bool {
  View::Bytes prefix = Lexicon::get_spelling(type);
  BAIL_IF(
      !begins_with(value, prefix) || value.get_size() <= prefix.get_size() ||
      value[value.get_size() - 1] != terminal);

  for (Count i = prefix.get_size(); i + 1 < value.get_size(); i++) {
    BAIL_IF(value[i] == terminal);
  }

  return True;
}

// String ranges allow an escaped byte to pass through without interpreting it.
// The final quote must remain unescaped so it closes the complete value.
static auto validate_string(View::Bytes value) -> Bool {
  View::Bytes prefix = Lexicon::get_spelling(Code::Type::String);
  BAIL_IF(
      !begins_with(value, prefix) || value.get_size() < 2 ||
      value[value.get_size() - 1] != '"');

  for (Count i = prefix.get_size(); i + 1 < value.get_size(); i++) {
    if (value[i] == '\\') {
      i++;
      BAIL_IF(i + 1 >= value.get_size());
      continue;
    }

    BAIL_IF(value[i] == '"');
  }

  return True;
}

static auto validate_attribute(View::Bytes value) -> Bool {
  View::Bytes prefix = Lexicon::get_spelling(Code::Type::Attribute);
  BAIL_IF(!begins_with(value, prefix));

  for (Count i = prefix.get_size(); i < value.get_size(); i++) {
    BAIL_IF(!Lexicon::is_identifier(value[i]));
  }

  return True;
}

static auto validate_comment(Code::Type type, View::Bytes value) -> Bool {
  View::Bytes prefix = Lexicon::get_spelling(type);
  BAIL_IF(!begins_with(value, prefix));
  if (type == Code::Type::Comment && value.get_size() > prefix.get_size() &&
      value[prefix.get_size()] == '/') {
    return False;
  }

  for (Count i = prefix.get_size(); i < value.get_size(); i++) {
    BAIL_IF(value[i] == '\n');
  }

  return True;
}

// Applies the one complete spelling rule owned by each supported Code.
static auto validate_code(Code::Type type, View::Bytes value) -> Bool {
  switch (type) {
  case Code::Type::Terminal:
    return value.is_empty();
  case Code::Type::Unknown:
  case Code::Type::PackedData:
    return False;
  case Code::Type::Type:
    return validate_type(value);
  case Code::Type::Addressable:
    return validate_addressable(value);
  case Code::Type::Attribute:
    return validate_attribute(value);
  case Code::Type::Numeric:
    return validate_numeric(value);
  case Code::Type::Float:
    return validate_float(value);
  case Code::Type::Hex:
    return validate_hex(value);
  case Code::Type::String:
    return validate_string(value);
  case Code::Type::Bytes:
    return validate_range(value, type, ']');
  case Code::Type::Embedded:
    return validate_range(value, type, ']');
  case Code::Type::Comment:
  case Code::Type::RawComment:
    return validate_comment(type, value);
  default: {
    View::Bytes fixed = Lexicon::get_spelling(type);
    return !fixed.is_empty() && value == fixed;
  }
  }
}

// Returns the longest allowed separator spelling at the current byte so
// overlapping operators follow the same preference as Tokenizer dispatch.
static auto separator_size(
    View::Bytes value,
    Count cursor,
    View::Vector<Code::Type> separators) -> Count {
  Count matched = 0;
  const auto* separator_data = separators.get_data();
  for (Count i = 0; i < separators.get_size(); i++) {
    View::Bytes spelling = Lexicon::get_spelling(separator_data[i]);
    if (spelling.get_size() <= matched ||
        cursor + spelling.get_size() > value.get_size()) {
      continue;
    }

    if (value.slice(cursor, spelling.get_size()) == spelling) {
      matched = spelling.get_size();
    }
  }

  return matched;
}

auto Lexicon::validate(
    Code::Type type,
    View::Bytes value,
    View::Vector<Code::Type> separators) -> Bool {
  if (separators.is_empty()) {
    return validate_code(type, value);
  }

  // Every separator must have one exact fixed spelling before parsing begins.
  const auto* separator_data = separators.get_data();
  for (Count i = 0; i < separators.get_size(); i++) {
    BAIL_IF(get_spelling(separator_data[i]).is_empty());
  }

  Count segment_start = 0;
  Count cursor = 0;
  while (cursor < value.get_size()) {
    Count matched = separator_size(value, cursor, separators);
    if (matched == 0) {
      cursor++;
      continue;
    }

    BAIL_IF(!validate_code(
        type, value.slice(segment_start, cursor - segment_start)));

    cursor += matched;
    segment_start = cursor;
  }

  return validate_code(type, value.slice(segment_start));
}
