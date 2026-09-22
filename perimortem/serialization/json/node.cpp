// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/serialization/json/node.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

enum class NodeState : U32 { Null, String, Number, Real, Object, Array, Flag };

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;

static auto hex_value(U8 value) -> S32 {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }

  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }

  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }

  return -1;
}

static auto read_hex_quad(View::Bytes source, Count position, U32& value)
    -> Bool {
  if (position + 4 > source.get_size()) {
    return False;
  }

  value = 0;
  for (Count i = 0; i < 4; i++) {
    S32 digit = hex_value(source[position + i]);
    if (digit < 0) {
      return False;
    }

    value = (value << 4) | U32(digit);
  }

  return True;
}

static auto is_high_surrogate(U32 value) -> Bool {
  return value >= 0xD800 && value <= 0xDBFF;
}

static auto is_low_surrogate(U32 value) -> Bool {
  return value >= 0xDC00 && value <= 0xDFFF;
}

static auto append_utf8(Managed::Bytes& output, U32 codepoint) -> void {
  if (codepoint <= 0x7F) {
    output.append(U8(codepoint));
    return;
  }

  if (codepoint <= 0x7FF) {
    output.append(U8(0xC0 | (codepoint >> 6)));
    output.append(U8(0x80 | (codepoint & 0x3F)));
    return;
  }

  if (codepoint <= 0xFFFF) {
    output.append(U8(0xE0 | (codepoint >> 12)));
    output.append(U8(0x80 | ((codepoint >> 6) & 0x3F)));
    output.append(U8(0x80 | (codepoint & 0x3F)));
    return;
  }

  output.append(U8(0xF0 | (codepoint >> 18)));
  output.append(U8(0x80 | ((codepoint >> 12) & 0x3F)));
  output.append(U8(0x80 | ((codepoint >> 6) & 0x3F)));
  output.append(U8(0x80 | (codepoint & 0x3F)));
}

static auto read_unicode_escape(
    View::Bytes source,
    Count slash,
    U32& codepoint,
    Count& consumed) -> Bool {
  if (slash + 6 > source.get_size() || source[slash] != '\\' ||
      source[slash + 1] != 'u') {
    return False;
  }

  U32 first = 0;
  if (!read_hex_quad(source, slash + 2, first)) {
    return False;
  }

  if (!is_high_surrogate(first)) {
    if (is_low_surrogate(first)) {
      return False;
    }

    codepoint = first;
    consumed = 6;
    return True;
  }

  if (slash + 12 > source.get_size() || source[slash + 6] != '\\' ||
      source[slash + 7] != 'u') {
    return False;
  }

  U32 second = 0;
  if (!read_hex_quad(source, slash + 8, second) || !is_low_surrogate(second)) {
    return False;
  }

  codepoint = 0x10000 + ((first - 0xD800) << 10) + (second - 0xDC00);
  consumed = 12;
  return True;
}

// Scans a string byte by byte so ascii escape characters are skipped.
auto parse_string(View::Bytes source, Count& position) -> View::Bytes {
  const auto start = ++position;
  auto data = source.get_data();
  while (position < source.get_size()) {
    switch (data[position]) {
    case '"': {
      auto result = source.slice(start, position - start);
      position++;
      return result;
    }

    case '\\':
      position += 2;
      break;
    default:
      position++;
      break;
    }
  }

  return source.slice(start);
}

auto ignored_characters(U8 c) {
  return c == ',' || c == '\n' || c == ' ';
}

auto Json::Node::set(const Core::View::Bytes value) -> void {
  data.ptr = value.get_data();
  data.size = value.get_size();
  data.state = (U32)NodeState::String;
}

auto Json::Node::set(const Core::View::Vector<Node> value) -> void {
  data.ptr = value.get_data();
  data.size = value.get_size();
  data.state = (U32)NodeState::Array;
}

auto Json::Node::set(const Core::View::Vector<Member> value) -> void {
  data.ptr = value.get_data();
  data.size = value.get_size();
  data.state = (U32)NodeState::Object;
}

auto Json::Node::set(S64 value) -> void {
  data.number = value;
  data.state = (U32)NodeState::Number;
}

auto Json::Node::set(R64 value) -> void {
  data.real = value;
  data.state = (U32)NodeState::Real;
}

auto Json::Node::set(Bool value) -> void {
  data.flag = value;
  data.state = (U32)NodeState::Flag;
}

auto Json::Node::set() -> void {
  data.state = (U32)NodeState::Null;
}

auto Json::Node::at(U32 index) const -> const Json::Node {
  if (data.state == (U32)NodeState::Array) {
    View::Vector<Json::Node> array((const Json::Node*)data.ptr, data.size);
    if (array.get_size() <= index) {
      return Json::Node();
    }

    return array.get_data()[index];
  }

  return Json::Node();
}

auto Json::Node::at(const View::Bytes name) const -> const Json::Node {
  if (data.state == (U32)NodeState::Object) {
    View::Vector<Member> members((const Member*)data.ptr, data.size);
    for (Count i = 0; i < members.get_size(); i++) {
      if (members.get_data()[i].name == name) {
        return members.get_data()[i].node;
      }
    }
  }

  return Json::Node();
}

auto Json::Node::operator[](U32 index) const -> const Json::Node {
  return at(index);
}

auto Json::Node::operator[](const View::Bytes name) const -> const Json::Node {
  return at(name);
}

auto Json::Node::contains(const View::Bytes name) const -> Bool {
  if (data.state == (U32)NodeState::Object) {
    View::Vector<Member> members((const Member*)data.ptr, data.size);
    return members.contains(
        [name](const Member& member) { return member.name == name; });
  }

  return false;
}

auto Json::Node::get_flag() const -> Bool {
  if (data.state == (U32)NodeState::Flag) {
    return data.flag;
  }

  return false;
}

auto Json::Node::get_number() const -> S64 {
  if (data.state == (U32)NodeState::Number) {
    return data.number;
  }

  return 0;
}

auto Json::Node::get_real() const -> double {
  if (data.state == (U32)NodeState::Real) {
    return data.real;
  }

  return 0.0;
}

auto Json::Node::get_string() const -> const View::Bytes {
  if (data.state == (U32)NodeState::String) {
    return View::Bytes((const U8*)data.ptr, data.size);
  }

  return View::Bytes();
}

auto Json::Node::decode_string(Allocator::Arena& arena) const -> View::Bytes {
  View::Bytes source = get_string();
  Managed::Bytes decoded(arena);
  for (Count i = 0; i < source.get_size(); i++) {
    if (source[i] != '\\' || i + 1 >= source.get_size()) {
      decoded.append(source[i]);
      continue;
    }

    Count slash = i;
    i++;
    switch (source[i]) {
    case 'n':
      decoded.append('\n');
      break;
    case 'r':
      decoded.append('\r');
      break;
    case 't':
      decoded.append('\t');
      break;
    case '"':
      decoded.append('"');
      break;
    case '\\':
      decoded.append('\\');
      break;
    case '/':
      decoded.append('/');
      break;
    case 'b':
      decoded.append('\b');
      break;
    case 'f':
      decoded.append('\f');
      break;
    case 'u': {
      U32 codepoint = 0;
      Count consumed = 0;
      if (read_unicode_escape(source, slash, codepoint, consumed)) {
        append_utf8(decoded, codepoint);
        i = slash + consumed - 1;
        break;
      }

      decoded.append('\\');
      decoded.append(source[i]);
      break;
    }
    default:
      decoded.append('\\');
      decoded.append(source[i]);
      break;
    }
  }

  return decoded.get_view();
}

auto Json::Node::get_array() const -> const View::Vector<Node> {
  if (data.state == (U32)NodeState::Array) {
    return View::Vector<Node>((const Node*)data.ptr, data.size);
  }

  return View::Vector<Node>();
}

auto Json::Node::get_object() const -> const View::Vector<Member> {
  if (data.state == (U32)NodeState::Object) {
    return View::Vector<Member>((const Member*)data.ptr, data.size);
  }

  return View::Vector<Member>();
}

auto Json::Node::get_size() const -> Count {
  switch ((NodeState)data.state) {
  case NodeState::String:
  case NodeState::Array:
  case NodeState::Object:
    return data.size;

  default:
    return 0;
  }
}

auto Json::Node::is_null() const -> Bool {
  return (NodeState)data.state == NodeState::Null;
}

auto Json::Node::is_flag() const -> Bool {
  return (NodeState)data.state == NodeState::Flag;
}

auto Json::Node::is_number() const -> Bool {
  return (NodeState)data.state == NodeState::Number;
}

auto Json::Node::is_real() const -> Bool {
  return (NodeState)data.state == NodeState::Real;
}

auto Json::Node::is_string() const -> Bool {
  return (NodeState)data.state == NodeState::String;
}

auto Json::Node::is_array() const -> Bool {
  return (NodeState)data.state == NodeState::Array;
}

auto Json::Node::is_object() const -> Bool {
  return (NodeState)data.state == NodeState::Object;
}

auto Json::Node::parse(
    Allocator::Arena& arena,
    View::Bytes source,
    Count position) -> Count {
  if (position > source.get_size()) {
    set();
    return position;
  }

  while (position < source.get_size()) {
    const auto start_char = source[position];
    switch (start_char) {
    // Object
    case '{': {
      Managed::Vector<Member> members(arena);
      position++;
      while (position < source.get_size()) {
        if (source[position] != '"') {
          if (source[position++] == '}') {
            break;
          }

          continue;
        }

        // Parse the name string.
        auto name = parse_string(source, position);

        // :
        position++;

        // Try to parse the child and if it fails propagate errors up the stack
        // and terminate parsing since we got garbage.
        Json::Node& child = arena.construct<Json::Node>();
        position = child.parse(arena, source, position);
        if (position == Count(-1)) {
          set();
          return Count(-1);
        }

        members.insert(Member(name, child));
      }

      set(members);
      return position;
    }

    // Array
    case '[': {
      Managed::Vector<Json::Node> array(arena);
      position++;
      while (position < source.get_size()) {
        if (ignored_characters(source[position])) {
          position++;
        }

        if (source[position] == ']') {
          break;
        }

        // Try to parse the child and if it fails propagate errors up the stack
        // and terminate parsing since we got garbage.
        Json::Node& child = arena.construct<Json::Node>();
        position = child.parse(arena, source, position);
        if (position == Count(-1)) {
          set();
          return Count(-1);
        }

        array.insert(child);
      }

      // If we are past the end then this is safe as we null out anyway.
      // If we are at a valid closing ']' then this consumes it.
      set(array.get_view());
      position++;
      return position;
    }

    // String
    case '"': {
      auto name = parse_string(source, position);
      set(name);
      return position;
    }

    // true
    case 't': {
      constexpr auto true_view = "true"_view;
      if (source.slice(position, true_view.get_size()) != "true"_view) {
        set();
        return Count(-1);
      }

      // Move past "true"
      position += true_view.get_size();

      set(True);
      return position;
    }

    // false
    case 'f': {
      constexpr auto false_view = "false"_view;
      if (source.slice(position, false_view.get_size()) != "false"_view) {
        set();
        return Count(-1);
      }

      // Move past "false"
      position += false_view.get_size();

      set(False);
      return position;
    }

    // Numbers
    case '-':
    case '0' ... '9': {
      Bool positive = True;
      if (source[position] == '-') {
        positive = False;
        position++;
      }

      S64 value = 0;
      while (position < source.get_size() && source[position] >= '0' &&
             source[position] <= '9') {
        value *= 10;
        value += source[position] - '0';
        position++;
      }

      if (position >= source.get_size() || source[position] != '.') {
        set(value * positive.sign());
        return position;
      }

      position++;  // consume '.'
      R64 float_value = value;
      U64 divisor = 1;

      // Try to perserve precision by using fixed point and only convert into
      // floating point once.
      while (position < source.get_size() && source[position] >= '0' &&
             source[position] <= '9') {
        float_value *= 10;
        divisor *= 10;
        float_value += source[position] - '0';
        position++;
      }

      set((float_value / R64(divisor)) * positive.sign());
      return position;
    }

    // null
    case 'n': {
      // If not null error out, otherwise it's actually a null value that we
      // need to store so continue parsing.
      set();

      constexpr auto null_view = "null"_view;
      if (source.slice(position, null_view.get_size()) != null_view) {
        return Count(-1);
      }

      position += null_view.get_size();
      return position;
    }

    default:
      position++;
      break;
    }
  }

  set();
  return position;
}

auto Json::Node::format(Allocator::Arena& arena) const -> View::Bytes {
  Managed::Bytes output(arena);
  Stream::Textual<Managed::Bytes> stream(output);

  auto inplace_format = [](this auto&& self,
                           Stream::Textual<Managed::Bytes>& stream,
                           const Json::Node& node) -> void {
    switch ((NodeState)node.data.state) {
    case NodeState::Null: {
      stream << "null"_view;
      return;
    }

    case NodeState::Array: {
      stream << "["_view;

      View::Vector<Json::Node> array = node.get_array();
      for (U32 i = 0; i < array.get_size(); i++) {
        self(stream, array.get_data()[i]);
        if (i != array.get_size() - 1) {
          stream << ","_view;
        }
      }

      stream << "]"_view;
      return;
    }

    case NodeState::Object: {
      stream << "{"_view;

      View::Vector<Member> members = node.get_object();
      for (U32 i = 0; i < members.get_size(); i++) {
        const auto& member = members.get_data()[i];
        stream << "\""_view << member.name << "\":"_view;

        self(stream, member.node);
        if (i != members.get_size() - 1) {
          stream << ","_view;
        }
      }

      stream << "}"_view;
      return;
    }

    case NodeState::String: {
      stream << "\""_view;
      constexpr auto hex_digits = "0123456789ABCDEF"_view;
      auto string = node.get_string();
      for (Count i = 0; i < string.get_size(); i++) {
        switch (string[i]) {
        case '"':
          stream << "\\\""_view;
          break;

        case '\\':
          stream << "\\\\"_view;
          break;

        case '\b':
          stream << "\\b"_view;
          break;

        case '\f':
          stream << "\\f"_view;
          break;

        case '\n':
          stream << "\\n"_view;
          break;

        case '\r':
          stream << "\\r"_view;
          break;

        case '\t':
          stream << "\\t"_view;
          break;

        default:
          if (string[i] >= 0x20) {
            stream << string.slice(i, 1);
            break;
          }

          stream << "\\u00"_view;
          stream << hex_digits.slice(string[i] >> 4, 1);
          stream << hex_digits.slice(string[i] & 0x0F, 1);
          break;
        }
      }

      stream << "\""_view;
      return;
    }

    case NodeState::Flag: {
      stream << node.data.flag;
      return;
    }

    case NodeState::Number: {
      stream << node.data.number;
      return;
    }

    case NodeState::Real: {
      stream << node.data.real;
      return;
    }
    }
  };

  // Start the format chain.
  inplace_format(stream, *this);

  // Return the output buffer.
  return output;
}
