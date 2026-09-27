// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/abi/representation/type_name.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

using namespace Perimortem;
using namespace Perimortem::Serialization;

static auto write_encoded(
    Stream::Textual<Memory::Managed::Bytes>& output,
    Core::View::Bytes value,
    Bool lowercase = False) -> void {
  constexpr auto hexadecimal = "0123456789abcdef"_view;
  for (Count index = 0; index < value.get_size(); index++) {
    U8 byte = value[index];
    Bool alphanumeric = Bool(
        (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
        (byte >= '0' && byte <= '9'));
    if (alphanumeric) {
      if (lowercase && byte >= 'A' && byte <= 'Z') {
        byte += 'a' - 'A';
      }
      output << Core::View::Bytes(&byte, 1);
      continue;
    }

    U8 encoded[] = {
      '_',
      hexadecimal[byte >> 4],
      hexadecimal[byte & 15],
    };
    output << Core::View::Bytes(encoded, 3);
  }
}

static auto write_package(
    Stream::Textual<Memory::Managed::Bytes>& output,
    Core::View::Bytes package) -> void {
  Count start = 0;
  for (Count index = 0; index <= package.get_size(); index++) {
    Bool end = index == package.get_size();
    if (!end && package[index] != '.') {
      continue;
    }
    if (start != 0) {
      output << "_"_view;
    }
    write_encoded(output, package.slice(start, index - start), True);
    start = index + 1;
  }
}

static auto write_route(
    Stream::Textual<Memory::Managed::Bytes>& output,
    Core::View::Bytes route) -> void {
  Count start = 0;
  for (Count index = 0; index <= route.get_size(); index++) {
    Bool end = index == route.get_size();
    Bool separator = !end && index + 1 < route.get_size() &&
                     route[index] == ':' && route[index + 1] == ':';
    if (!end && !separator) {
      continue;
    }
    if (start != 0) {
      output << "_"_view;
    }
    write_encoded(output, route.slice(start, index - start));
    if (separator) {
      index++;
    }
    start = index + 1;
  }
}

auto Tetrodotoxin::Terminal::Abi::Representation::TypeName::create(
    Memory::Allocator::Arena& arena,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Source::Type& type,
    Core::View::Bytes inherited_package,
    Core::View::Bytes inherited_member)
    -> Core::Option<Tetrodotoxin::Terminal::Abi::Representation::TypeName> {
  Core::View::Bytes package = inherited_package;
  Core::View::Bytes member = inherited_member;
  Core::View::Bytes route;
  auto binding = unit.find_type(type);
  if (binding) {
    package = binding->get_package();
    member = binding->get_member();
    route = binding->get_route();
  } else {
    if (package.is_empty()) {
      package = unit.get_package();
    }
    if (member.is_empty()) {
      member = unit.get_member();
    }
    Memory::Managed::Bytes fallback(arena, member);
    if (!fallback.get_view().is_empty()) {
      fallback.concat("::"_view);
    }
    fallback.concat(type.get_name());
    route = fallback.get_view();
  }
  if (package.is_empty() || route.is_empty()) {
    return {};
  }

  Memory::Managed::Bytes rendered(arena);
  Stream::Textual<Memory::Managed::Bytes> output(rendered);
  output << "ttx_"_view;
  write_package(output, package);
  output << "_"_view;
  write_route(output, route);
  return TypeName(type, rendered.get_view(), package, member);
}
