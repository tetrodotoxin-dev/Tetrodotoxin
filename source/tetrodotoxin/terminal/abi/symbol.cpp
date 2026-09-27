// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/abi/symbol.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx;

static auto append_encoded_name(
    Memory::Managed::Bytes& output,
    Core::View::Bytes value) -> void {
  constexpr auto digits = "0123456789abcdef"_view;

  for (Count index = 0; index < value.get_size(); index++) {
    U8 byte = value[index];
    Bool alphanumeric = Bool(
        (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
        (byte >= '0' && byte <= '9'));
    if (alphanumeric) {
      output.append(byte);
    } else {
      output.append('_');
      output.append(digits[byte >> 4]);
      output.append(digits[byte & 15]);
    }
  }
}

static auto append_symbol_path(
    Memory::Managed::Bytes& output,
    const Tetrodotoxin::Source::Abstract& value,
    Count path_start) -> void {
  if (value.is<Tetrodotoxin::Language::Monograph>()) {
    return;
  }

  auto function = value.select<Tetrodotoxin::Library::Language::Function>();
  if (function) {
    append_symbol_path(
        output, function->get_definition().get_host(), path_start);
    if (output.get_size() != path_start) {
      output.concat("__"_view);
    }

    append_encoded_name(output, function->get_name());
    return;
  }

  auto field = value.select<Tetrodotoxin::Library::Language::Field>();
  if (field) {
    append_symbol_path(output, field->get_definition().get_host(), path_start);
    if (output.get_size() != path_start) {
      output.concat("__"_view);
    }

    append_encoded_name(output, field->get_name());
    return;
  }

  auto composite =
      value.select<Tetrodotoxin::Library::Language::Types::Composite>();
  if (composite) {
    if (composite->is<Tetrodotoxin::Library::Language::Types::Source>()) {
      return;
    }

    append_symbol_path(
        output, composite->get_definition().get_host(), path_start);
    if (output.get_size() != path_start) {
      output.concat("__"_view);
    }

    append_encoded_name(output, composite->get_name());
    return;
  }

  auto enumeration =
      value.select<Tetrodotoxin::Library::Language::Types::Enumeration>();
  if (enumeration) {
    append_symbol_path(
        output, enumeration->get_definition().get_host(), path_start);
    if (output.get_size() != path_start) {
      output.concat("__"_view);
    }

    append_encoded_name(output, enumeration->get_name());
    return;
  }

  append_encoded_name(output, value.get_name());
}

auto Tetrodotoxin::Terminal::Abi::Symbol::validate(Core::View::Bytes value)
    -> Bool {
  if (value.is_empty()) {
    return False;
  }

  for (Count index = 0; index < value.get_size(); index++) {
    U8 byte = value[index];
    Bool letter =
        Bool((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z'));
    Bool valid = Bool(
        letter || byte == '_' || (index != 0 && byte >= '0' && byte <= '9'));
    if (!valid) {
      return False;
    }
  }

  return True;
}

Tetrodotoxin::Terminal::Abi::Symbol::Symbol(
    Memory::Allocator::Arena& arena,
    const Tetrodotoxin::Source::Abstract& semantic,
    Kind kind,
    Unit unit) {
  Memory::Managed::Bytes output(arena);
  switch (kind) {
  case Kind::Path:
    break;
  case Kind::FunctionStatic:
  case Kind::FunctionSelf:
  case Kind::Construction:
    output.concat("TTX_FUNC_"_view);
    break;
  case Kind::Address:
    output.concat("TTX_ADDR_"_view);
    break;
  case Kind::ReadOnly:
    output.concat("TTX_DATA_"_view);
    break;
  case Kind::ObjectDescriptor:
    output.concat("TTX_DESC_"_view);
    break;
  case Kind::Projection:
    output.concat("TTX_PROJ_"_view);
    break;
  case Kind::GraphicsChildren:
    output.concat("TTX_GFX_"_view);
    break;
  }

  Count path_start = output.get_size();
  Bool published_identity = kind != Kind::Path;
  Core::View::Bytes package = unit.get_package();
  Core::View::Bytes member = unit.get_member();
  auto selected_type = semantic.select<Tetrodotoxin::Source::Type>();
  const Tetrodotoxin::Source::Type* type = selected_type ? &*selected_type : nullptr;
  auto function = semantic.select<Tetrodotoxin::Library::Language::Function>();
  if (function) {
    type = &function->get_host();
  }
  auto field = semantic.select<Tetrodotoxin::Library::Language::Field>();
  if (field) {
    auto field_host =
        field->get_definition().get_host().select<Tetrodotoxin::Source::Type>();
    type = field_host ? &*field_host : nullptr;
  }
  auto binding =
      type ? unit.find_type(*type) : Core::Option<const Unit::TypeBinding&>();
  if (binding) {
    package = binding->get_package();
    member = binding->get_member();
  }
  if (published_identity && !package.is_empty() && !member.is_empty()) {
    append_encoded_name(output, package);
    output.concat("__"_view);
    append_encoded_name(output, member);
    output.concat("__"_view);
    path_start = output.get_size();
  }
  append_symbol_path(output, semantic, path_start);
  if (kind == Kind::FunctionStatic) {
    output.concat("_static"_view);
  } else if (kind == Kind::FunctionSelf) {
    output.concat("_self"_view);
  } else if (kind == Kind::Construction) {
    output.concat("__construct_static"_view);
  }

  value = output.get_view();
}
