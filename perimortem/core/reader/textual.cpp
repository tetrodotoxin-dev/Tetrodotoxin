// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/reader/textual.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;

static constexpr U64 signed_maximum = U64(-1) >> 1;
static constexpr U64 signed_minimum_magnitude = signed_maximum + 1;
static constexpr S64 signed_minimum = S64(-9223372036854775807LL - 1);

static constexpr auto get_digit(U8 character) -> U8 {
  if (character >= '0' && character <= '9') {
    return character - '0';
  }
  if (character >= 'A' && character <= 'F') {
    return character - 'A' + 10;
  }
  if (character >= 'a' && character <= 'f') {
    return character - 'a' + 10;
  }
  return U8(-1);
}

static auto
    parse_unsigned(View::Bytes source, Count& cursor, U8 radix, U64 limit)
        -> U64 {
  if (radix < 2 || radix > 16 || cursor >= source.get_size()) [[unlikely]] {
    cursor = Count(-1);
    return 0;
  }

  U8 first = get_digit(source[cursor]);
  if (first >= radix) [[unlikely]] {
    cursor = Count(-1);
    return 0;
  }

  U64 result = 0;
  while (cursor < source.get_size()) {
    U8 digit = get_digit(source[cursor]);
    if (digit >= radix) {
      break;
    }

    if (result > (limit - digit) / radix) [[unlikely]] {
      cursor = Count(-1);
      return 0;
    }

    result = result * radix + digit;
    cursor++;
  }

  return result;
}

static auto skip_whitespace(View::Bytes source, Count& cursor) -> void {
  // Outer loop already does a bounds check so grab the raw pointer.
  auto text = source.get_data();
  while (cursor < source.get_size()) {
    U8 value = text[cursor];
    if (value != ' ' && value != '\n' && value != '\r' && value != '\t') {
      break;
    }

    cursor++;
  }
}

auto Reader::Textual::read_byte() -> U8 {
  if (!has_content()) [[unlikely]] {
    cursor = Count(-1);
    return U8(0);
  }

  return source.get_data()[cursor++];
}

auto Reader::Textual::read_flag() -> Bool {
  skip_whitespace(source, cursor);
  if (!has_content()) [[unlikely]] {
    cursor = Count(-1);
    return false;
  }

  // Use a switch for cheap case insensitivity.
  switch (source.get_data()[cursor]) {
  case 'T':
  case 't':
    if (source.slice(cursor + 1, 3) == "rue"_view) {
      cursor += 4;
      return true;
    } else {
      cursor = Count(-1);
      return false;
    }

  case 'F':
  case 'f':
    if (source.slice(cursor + 1, 4) == "alse"_view) {
      cursor += 5;
    } else {
      cursor = Count(-1);
    }

    return false;

  default:
    cursor = Count(-1);
    return false;
  }
}

auto Reader::Textual::read_unsigned(U8 radix) -> U64 {
  skip_whitespace(source, cursor);
  return parse_unsigned(source, cursor, radix, U64(-1));
}

auto Reader::Textual::read_signed() -> S64 {
  skip_whitespace(source, cursor);
  if (!has_content()) [[unlikely]] {
    cursor = Count(-1);
    return 0;
  }

  Bool negative = source[cursor] == '-';
  if (negative) {
    cursor++;
  }

  U64 limit = negative ? signed_minimum_magnitude : signed_maximum;
  U64 magnitude = parse_unsigned(source, cursor, 10, limit);
  if (!is_valid()) {
    return 0;
  }

  if (magnitude == signed_minimum_magnitude) {
    return signed_minimum;
  }
  return negative ? -S64(magnitude) : S64(magnitude);
}

auto Reader::Textual::read_r32() -> R32 {
  R64 wide = read_r64();
  if (!is_valid()) {
    return 0;
  }

  R32 value = R32(wide);
  if (!__builtin_isfinite(value) || (wide != 0 && value == 0)) [[unlikely]] {
    cursor = Count(-1);
    return 0;
  }
  return value;
}

auto Reader::Textual::read_r64() -> R64 {
  skip_whitespace(source, cursor);
  if (!has_content()) [[unlikely]] {
    cursor = Count(-1);
    return R64(0);
  }

  Bool negative = False;
  if (source[cursor] == '-') {
    negative = True;
    cursor++;
  }

  if (!has_content() || get_digit(source[cursor]) > 9) [[unlikely]] {
    cursor = Count(-1);
    return 0;
  }

  R64 result = 0;
  Bool nonzero = False;
  while (has_content()) {
    U8 digit = get_digit(source[cursor]);
    if (digit > 9) {
      break;
    }

    nonzero |= digit != 0;
    result = result * 10 + digit;
    if (!__builtin_isfinite(result)) [[unlikely]] {
      cursor = Count(-1);
      return 0;
    }
    cursor++;
  }

  if (!has_content()) [[unlikely]] {
    return negative ? -result : result;
  }

  // A decimal point ends the integer portion. Any later byte belongs to the
  // next greedy read unless it is another decimal digit.
  auto data = source.get_data();
  if (data[cursor] != '.') {
    return negative ? -result : result;
  }

  R64 fraction = 0.1;
  cursor++;
  while (has_content()) {
    U8 digit = get_digit(data[cursor]);
    if (digit > 9) {
      break;
    }

    nonzero |= digit != 0;
    result += R64(digit) * fraction;
    fraction *= 0.1;
    cursor++;
  }

  if (nonzero && result == 0) [[unlikely]] {
    cursor = Count(-1);
    return 0;
  }
  return negative ? -result : result;
}
