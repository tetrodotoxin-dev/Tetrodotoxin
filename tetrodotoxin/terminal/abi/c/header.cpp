// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/abi/c/header.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "perimortem/abi/core/object.hpp"
#include "tetrodotoxin/library/language/foreign.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/terminal/abi/representation/type_name.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/callable.hpp"
#include "tetrodotoxin/source/type.hpp"

using namespace Perimortem;
using namespace Perimortem::Serialization;

static constexpr Core::View::Bytes header_opening =
    "// # Tetrodotoxin\n"
    "// Copyright (c) 2023-present Matt Kaes and contributors\n\n"
    "#pragma once\n\n"_view;

class HeaderOrigin {
 public:
  constexpr HeaderOrigin(
      Core::View::Bytes package = {},
      Core::View::Bytes member = {})
      : package(package), member(member) {}

  constexpr auto get_package() const -> Core::View::Bytes { return package; }

  constexpr auto get_member() const -> Core::View::Bytes { return member; }

 private:
  Core::View::Bytes package;
  Core::View::Bytes member;
};

static auto fail_header(Core::View::Bytes message) -> Bool {
  Core::Diagnostics::Log::error(message);
  return False;
}

static auto require_type(const Tetrodotoxin::Source::Abstract& answer)
    -> Core::Option<const Tetrodotoxin::Source::Type&> {
  auto direct = answer.select<Tetrodotoxin::Source::Type>();
  return direct ? direct : answer.resolve().select<Tetrodotoxin::Source::Type>();
}

static auto require_addressable_type(const Tetrodotoxin::Source::Addressable& addressable)
    -> Core::Option<const Tetrodotoxin::Source::Type&> {
  return require_type(addressable.get_type());
}

static auto require_result_type(const Tetrodotoxin::Source::Layout& layout, Count index)
    -> Core::Option<const Tetrodotoxin::Source::Type&> {
  auto entry = layout.get_abstract(index);
  auto addressable = entry ? entry->select<Tetrodotoxin::Source::Addressable>()
                           : Core::Option<const Tetrodotoxin::Source::Addressable&>();
  return addressable ? require_addressable_type(*addressable)
         : entry     ? require_type(*entry)
                     : Core::Option<const Tetrodotoxin::Source::Type&>();
}

static auto require_parameter(const Tetrodotoxin::Source::Layout& layout, Count index)
    -> Core::Option<const Tetrodotoxin::Source::Addressable&> {
  auto entry = layout.get_abstract(index);
  return entry ? entry->select<Tetrodotoxin::Source::Addressable>()
               : Core::Option<const Tetrodotoxin::Source::Addressable&>();
}

static auto write_encoded_name(
    Stream::Textual<Memory::Managed::Bytes>& output,
    Core::View::Bytes value,
    Bool lowercase = False) -> void {
  constexpr auto hex = "0123456789abcdef"_view;
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
    } else {
      Core::Static::Vector<U8, 3> encoded = {{
        '_',
        hex[byte >> 4],
        hex[byte & 15],
      }};
      output << Core::View::Bytes(encoded.get_data(), encoded.get_size());
    }
  }
}

static auto write_package_name(
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
    write_encoded_name(output, package.slice(start, index - start), True);
    start = index + 1;
  }
}

static auto find_header_name(
    const Memory::Managed::Vector<
        Tetrodotoxin::Terminal::Abi::Representation::TypeName>& names,
    const Tetrodotoxin::Source::Type& type)
    -> Core::Option<
        const Tetrodotoxin::Terminal::Abi::Representation::TypeName&> {
  auto retained = names.get_view();
  for (Count index = 0; index < retained.get_size(); index++) {
    const Tetrodotoxin::Terminal::Abi::Representation::TypeName& name =
        retained.get_data()[index];
    if (&name.get_type() == &type) {
      return name;
    }
  }
  return {};
}

static auto create_header_name(
    Memory::Allocator::Arena& arena,
    Memory::Managed::Vector<
        Tetrodotoxin::Terminal::Abi::Representation::TypeName>& names,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Source::Type& type,
    HeaderOrigin inherited) -> Core::Option<HeaderOrigin> {
  auto retained = find_header_name(names, type);
  if (retained) {
    return HeaderOrigin(retained->get_package(), retained->get_member());
  }

  auto name = Tetrodotoxin::Terminal::Abi::Representation::TypeName::create(
      arena, unit, type, inherited.get_package(), inherited.get_member());
  if (!name) {
    return {};
  }
  for (const Tetrodotoxin::Terminal::Abi::Representation::TypeName& existing :
       names.get_view()) {
    if (existing.get_value() == name->get_value() &&
        &existing.get_type() != &type) {
      return {};
    }
  }
  HeaderOrigin origin(name->get_package(), name->get_member());
  names.insert(*name);
  return origin;
}

static auto collect_type(
    Memory::Allocator::Arena& arena,
    Memory::Managed::Vector<const Tetrodotoxin::Source::Type*>& ordered,
    Memory::Managed::Map<const Tetrodotoxin::Source::Type*, Bool>& collected,
    Memory::Managed::Vector<
        Tetrodotoxin::Terminal::Abi::Representation::TypeName>& names,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Source::Type& type,
    HeaderOrigin inherited) -> Bool {
  if (collected.contains(&type)) {
    return True;
  }

  auto kind = types.get_kind(type);
  if (!kind) {
    return fail_header(
        "The C header cannot find a completed carrier for a published Type."_view);
  }

  HeaderOrigin origin = inherited;
  if (*kind != Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Value &&
      *kind != Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                   Enumeration &&
      *kind !=
          Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Context) {
    auto created = create_header_name(arena, names, unit, type, inherited);
    if (!created) {
      return fail_header(
          "The C header cannot create one unique Package qualified carrier name."_view);
    }
    origin = *created;
  }

  collected.insert(&type, True);
  switch (*kind) {
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Value:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Implementation:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::ObjectStorage:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object:
    ordered.insert(&type);
    return True;

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Structure: {
    auto fields = types.get_fields(type);
    if (!fields) {
      return fail_header(
          "The C header cannot find the completed Structure fields."_view);
    }

    for (Count index = 0; index < fields->get_size(); index++) {
      auto field = require_parameter(*fields, index);
      auto field_type = field ? require_addressable_type(*field)
                              : Core::Option<const Tetrodotoxin::Source::Type&>();
      if (!field || !field_type ||
          !collect_type(
              arena, ordered, collected, names, types, unit, *field_type,
              origin)) {
        return fail_header(
            "The C header found a Structure field without a completed carrier."_view);
      }
    }

    break;
  }

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Result: {
    auto value = types.get_element(type);
    auto error = types.get_error(type);
    if (!value || !error ||
        !collect_type(
            arena, ordered, collected, names, types, unit, *value, origin) ||
        !collect_type(
            arena, ordered, collected, names, types, unit, *error, origin)) {
      return fail_header(
          "The C header found Result without both completed alternatives."_view);
    }

    break;
  }

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Enumeration:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Fixed:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Option:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Range:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::View:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Access: {
    auto element = types.get_element(type);
    if (!element ||
        !collect_type(
            arena, ordered, collected, names, types, unit, *element, origin)) {
      return fail_header(
          "The C header found a carrier without its completed element Type."_view);
    }

    break;
  }

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Context:
    return fail_header(
        "The C header cannot publish a contextual Type as a C value."_view);
  }

  ordered.insert(&type);
  return True;
}

static auto collect_callable(
    Memory::Allocator::Arena& arena,
    Memory::Managed::Vector<const Tetrodotoxin::Source::Type*>& ordered,
    Memory::Managed::Map<const Tetrodotoxin::Source::Type*, Bool>& collected,
    Memory::Managed::Vector<
        Tetrodotoxin::Terminal::Abi::Representation::TypeName>& names,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Source::Callable& callable) -> Bool {
  HeaderOrigin origin(unit.get_package(), unit.get_member());
  const Tetrodotoxin::Source::Layout& results = callable.get_results();
  for (Count index = 0; index < results.get_size(); index++) {
    auto type = require_result_type(results, index);
    if (!type ||
        !collect_type(
            arena, ordered, collected, names, types, unit, *type, origin)) {
      return fail_header(
          "The C header found a Callable result without a completed carrier."_view);
    }
  }

  const Tetrodotoxin::Source::Layout& parameters = callable.get_parameters();
  for (Count index = 0; index < parameters.get_size(); index++) {
    auto parameter = require_parameter(parameters, index);
    auto parameter_type = parameter ? require_addressable_type(*parameter)
                                    : Core::Option<const Tetrodotoxin::Source::Type&>();
    if (!parameter || !parameter_type ||
        !collect_type(
            arena, ordered, collected, names, types, unit, *parameter_type,
            origin)) {
      return fail_header(
          "The C header found a Callable parameter without a completed carrier."_view);
    }
  }

  return True;
}

static auto write_type_name(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Memory::Managed::Vector<
        Tetrodotoxin::Terminal::Abi::Representation::TypeName>& names,
    const Tetrodotoxin::Source::Type& type) -> Bool {
  auto kind = types.get_kind(type);
  if (!kind) {
    return fail_header(
        "The C header cannot name a Type without its completed carrier."_view);
  }

  switch (*kind) {
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Value: {
    auto width = types.get_width(type);
    if (!width) {
      return fail_header(
          "The C header cannot name a scalar without its physical width."_view);
    }

    if (*width == 1 && !types.is_real(type)) {
      output << "bool"_view;
    } else if (types.is_real(type) && *width == 32) {
      output << "float"_view;
    } else if (types.is_real(type) && *width == 64) {
      output << "double"_view;
    } else if (types.is_real(type)) {
      return fail_header(
          "The C header cannot name the selected Real carrier width."_view);
    } else if (types.is_signed(type)) {
      output << "int"_view << *width << "_t"_view;
    } else {
      output << "uint"_view << *width << "_t"_view;
    }

    return True;
  }

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Enumeration: {
    auto storage = types.get_element(type);
    return storage && write_type_name(output, types, names, *storage);
  }

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Context:
    return fail_header(
        "The C header cannot name a contextual Type as a C value."_view);

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Fixed:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Option:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Result:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Range:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::View:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Access:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Implementation:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Structure:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::ObjectStorage:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object: {
    auto name = find_header_name(names, type);
    if (!name) {
      return fail_header(
          "The C header cannot find one Package qualified carrier name."_view);
    }
    output << name->get_value();
    return True;
  }
  }
}

static auto write_type_definition(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Memory::Managed::Vector<
        Tetrodotoxin::Terminal::Abi::Representation::TypeName>& names,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Source::Type& type,
    Bool& uses_objects) -> Bool {
  auto kind = types.get_kind(type);
  if (!kind) {
    return fail_header(
        "The C header cannot define a Type without its completed carrier."_view);
  }

  if (*kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Value ||
      *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                   Enumeration) {
    return True;
  }
  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Context) {
    return fail_header(
        "The C header found an unsupported physical carrier."_view);
  }
  auto retained_name = find_header_name(names, type);
  if (!retained_name) {
    return fail_header(
        "The C header cannot define one unnamed physical carrier."_view);
  }
  if (retained_name->get_package() != unit.get_package()) {
    return True;
  }
  Core::View::Bytes type_name = retained_name->get_value();
  output << "#ifndef TTX_CARRIER_"_view << type_name
         << "\n#define TTX_CARRIER_"_view << type_name << " 1\n"_view;

  switch (*kind) {
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Value:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Enumeration:
    return True;

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::ObjectStorage:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object:
    output << "typedef struct "_view << type_name << "_object *"_view
           << type_name << ";\n#endif\n\n"_view;
    uses_objects = True;
    return True;

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Implementation:
    output << "typedef struct "_view << type_name
           << " {\n  void *object;\n  const void *projection;\n} "_view
           << type_name << ";\n#endif\n\n"_view;
    uses_objects = True;
    return True;

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Structure: {
    output << "typedef struct "_view << type_name << " {\n"_view;

    auto fields = types.get_fields(type);
    if (!fields) {
      return fail_header(
          "The C header cannot define a Structure without its fields."_view);
    }

    for (Count index = 0; index < fields->get_size(); index++) {
      auto field = require_parameter(*fields, index);
      auto field_type = field ? require_addressable_type(*field)
                              : Core::Option<const Tetrodotoxin::Source::Type&>();
      auto name = fields->get_name(index);
      if (!field || !field_type) {
        return fail_header(
            "The C header found a Structure field without an Addressable."_view);
      }

      output << "  "_view;
      if (!write_type_name(output, types, names, *field_type)) {
        return False;
      }

      output << " "_view;
      write_encoded_name(output, name ? *name : field->get_name());
      output << ";\n"_view;
    }

    break;
  }

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Fixed: {
    output << "typedef struct "_view << type_name << " {\n"_view;

    auto element = types.get_element(type);
    auto extent = types.get_extent(type);
    if (!element || !extent) {
      return fail_header(
          "The C header cannot define Fixed without its element and extent."_view);
    }

    output << "  "_view;
    if (!write_type_name(output, types, names, *element)) {
      return False;
    }

    output << " values["_view << *extent << "];\n"_view;
    break;
  }

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::View:
  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Access: {
    output << "typedef struct "_view << type_name << " {\n"_view;

    auto element = types.get_element(type);
    if (!element) {
      return fail_header(
          "The C header cannot define contiguous storage without its element."_view);
    }

    output << "  "_view;
    if (*kind ==
        Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::View) {
      output << "const "_view;
    }

    if (!write_type_name(output, types, names, *element)) {
      return False;
    }

    output << " *data;\n  uint64_t size;\n"_view;
    break;
  }

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Range: {
    output << "typedef struct "_view << type_name << " {\n"_view;

    auto element = types.get_element(type);
    if (!element) {
      return fail_header(
          "The C header cannot define Range without its element."_view);
    }

    output << "  "_view;
    if (!write_type_name(output, types, names, *element)) {
      return False;
    }

    output << " start;\n  "_view;
    if (!write_type_name(output, types, names, *element)) {
      return False;
    }

    output << " end;\n"_view;
    break;
  }

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Option: {
    auto element = types.get_element(type);
    if (!element) {
      return fail_header(
          "The C header cannot define Option without its element."_view);
    }

    if (types.is_object(*element)) {
      output << "typedef "_view;
      if (!write_type_name(output, types, names, *element)) {
        return False;
      }

      output << " "_view << type_name << ";\n#endif\n\n"_view;
      return True;
    }

    output << "typedef struct "_view << type_name << " {\n"_view;

    output << "  "_view;
    if (!write_type_name(output, types, names, *element)) {
      return False;
    }

    output << " value;\n  bool set;\n"_view;
    break;
  }

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Result: {
    output << "typedef struct "_view << type_name << " {\n  union {\n    "_view;

    auto value = types.get_element(type);
    auto error = types.get_error(type);
    if (!value || !error || !write_type_name(output, types, names, *value)) {
      return fail_header(
          "The C header cannot define Result without its value Type."_view);
    }

    output << " value;\n    "_view;
    if (!write_type_name(output, types, names, *error)) {
      return False;
    }

    output << " error;\n  };\n  bool value_selected;\n"_view;
    break;
  }

  case Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Context:
    return False;
  }

  output << "} "_view << type_name << ";\n#endif\n\n"_view;
  return True;
}

static auto write_result_name(
    Stream::Textual<Memory::Managed::Bytes>& output,
    Core::View::Bytes symbol) -> void {
  output << "ttx_results_"_view;
  write_encoded_name(output, symbol);
}

static auto write_result_definition(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Memory::Managed::Vector<
        Tetrodotoxin::Terminal::Abi::Representation::TypeName>& names,
    const Tetrodotoxin::Source::Callable& callable,
    Core::View::Bytes symbol) -> Bool {
  const Tetrodotoxin::Source::Layout& results = callable.get_results();
  if (results.get_size() < 2) {
    return True;
  }

  output << "typedef struct "_view;
  write_result_name(output, symbol);
  output << " {\n"_view;
  for (Count index = 0; index < results.get_size(); index++) {
    auto type = require_result_type(results, index);
    if (!type) {
      return fail_header(
          "The C header found a result without an exact Type."_view);
    }

    output << "  "_view;
    if (!write_type_name(output, types, names, *type)) {
      return False;
    }

    output << " "_view;
    auto name = results.get_name(index);
    if (name) {
      write_encoded_name(output, *name);
    } else {
      output << "value"_view << index;
    }

    output << ";\n"_view;
  }

  output << "} "_view;
  write_result_name(output, symbol);
  output << ";\n\n"_view;
  return True;
}

static auto declares_self(const Tetrodotoxin::Source::Callable& callable) -> Bool {
  auto first = callable.get_parameters().get_abstract(0);
  auto parameter = first ? first->select<Tetrodotoxin::Source::Addressable>()
                         : Core::Option<const Tetrodotoxin::Source::Addressable&>();
  return parameter && parameter->get_name() == "self"_view;
}

static auto write_signature(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Memory::Managed::Vector<
        Tetrodotoxin::Terminal::Abi::Representation::TypeName>& names,
    const Tetrodotoxin::Source::Callable& callable,
    Core::View::Bytes symbol) -> Bool {
  const Tetrodotoxin::Source::Layout& results = callable.get_results();
  if (results.is_empty()) {
    output << "void"_view;
  } else if (results.get_size() == 1) {
    auto result = require_result_type(results, 0);
    if (!result || !write_type_name(output, types, names, *result)) {
      return fail_header(
          "The C header found a result without an exact carrier."_view);
    }
    auto entry = results.get_abstract(0);
    if (entry && entry->is<Tetrodotoxin::Source::Addressable>()) {
      output << "*"_view;
    }
  } else {
    write_result_name(output, symbol);
  }

  output << " "_view << symbol << "("_view;
  const Tetrodotoxin::Source::Layout& parameters = callable.get_parameters();
  if (parameters.is_empty()) {
    output << "void"_view;
  }

  for (Count index = 0; index < parameters.get_size(); index++) {
    if (index != 0) {
      output << ", "_view;
    }

    auto parameter = require_parameter(parameters, index);
    auto parameter_type = parameter ? require_addressable_type(*parameter)
                                    : Core::Option<const Tetrodotoxin::Source::Type&>();
    if (!parameter || !parameter_type ||
        !write_type_name(output, types, names, *parameter_type)) {
      return fail_header(
          "The C header found a parameter without an exact carrier."_view);
    }

    if (index == 0 && declares_self(callable)) {
      output << "*"_view;
    }
    output << " "_view;
    auto name = parameters.get_name(index);
    if (name) {
      write_encoded_name(output, *name);
    } else {
      output << "value"_view << index;
    }
  }

  output << ");\n"_view;
  return True;
}

auto Tetrodotoxin::Terminal::Abi::C::Header::create(
    Memory::Allocator::Arena& arena,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Library::Language::Monograph& monograph,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Export> exports,
    Core::View::Vector<Tetrodotoxin::Source::Reference<
        const Tetrodotoxin::Library::Language::Model::Type>> roots)
    -> Core::Option<Tetrodotoxin::Terminal::Abi::C::Header> {
  Memory::Managed::Vector<const Tetrodotoxin::Source::Type*> ordered(arena);
  Memory::Managed::Map<const Tetrodotoxin::Source::Type*, Bool> collected(arena);
  Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Representation::TypeName>
      names(arena);

  auto collect_root = [&](const Tetrodotoxin::Source::Type& type) -> Bool {
    auto kind = types.get_kind(type);
    return !kind ||
           *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                        Context ||
           collect_type(
               arena, ordered, collected, names, types, unit, type,
               HeaderOrigin(unit.get_package(), unit.get_member()));
  };
  if (roots.is_empty()) {
    for (const Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract>& declaration :
         monograph.get_source().get_types(
             Tetrodotoxin::Language::Visibility::Public)) {
      auto type = declaration.get().select<Tetrodotoxin::Source::Type>();
      if (type && !collect_root(*type)) {
        return {};
      }
    }
  } else {
    for (const Tetrodotoxin::Source::Reference<
             const Tetrodotoxin::Library::Language::Model::Type>& root :
         roots) {
      if (!collect_root(root.get())) {
        return {};
      }
    }
  }

  for (const Tetrodotoxin::Terminal::Abi::Export& exported : exports) {
    if (!collect_callable(
            arena, ordered, collected, names, types, unit,
            exported.get_callable())) {
      return {};
    }
  }

  const Tetrodotoxin::Library::Language::Foreign& foreign =
      monograph.get_source().get_foreign();
  for (const Tetrodotoxin::Source::Reference<
           Tetrodotoxin::Library::Language::Foreign::State>& retained :
       foreign.get_states()) {
    const Tetrodotoxin::Source::Addressable& addressable = retained.get();
    auto type = require_addressable_type(addressable);
    if (!type || !collect_type(
                     arena, ordered, collected, names, types, unit, *type,
                     HeaderOrigin(unit.get_package(), unit.get_member()))) {
      return {};
    }
  }

  for (const Tetrodotoxin::Source::Reference<
           Tetrodotoxin::Library::Language::Foreign::Function>& retained :
       foreign.get_functions()) {
    const Tetrodotoxin::Source::Callable& callable = retained.get();
    if (!collect_callable(
            arena, ordered, collected, names, types, unit, callable)) {
      return {};
    }
  }

  Memory::Managed::Bytes buffer(arena);
  Stream::Textual<Memory::Managed::Bytes> output(buffer);
  output << header_opening
         << "#include <stdbool.h>\n#include <stdint.h>\n\n"_view;
  for (Core::View::Bytes header : unit.get_headers()) {
    output << "#include \""_view << header << "\"\n"_view;
  }
  if (!unit.get_headers().is_empty()) {
    output << "\n"_view;
  }
  Bool uses_objects = False;
  for (const Tetrodotoxin::Source::Type* type : ordered.get_view()) {
    if (!write_type_definition(
            output, types, names, unit, *type, uses_objects)) {
      return {};
    }
  }

  for (const Tetrodotoxin::Terminal::Abi::Export& exported : exports) {
    if (!write_result_definition(
            output, types, names, exported.get_callable(),
            exported.get_symbol())) {
      return {};
    }
  }

  for (const Tetrodotoxin::Source::Reference<
           Tetrodotoxin::Library::Language::Foreign::Function>& retained :
       foreign.get_functions()) {
    const Tetrodotoxin::Library::Language::Foreign::Function& callable =
        retained.get();
    if (!write_result_definition(
            output, types, names, callable, callable.get_symbol())) {
      return {};
    }
  }

  output << "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n"_view;
  if (uses_objects) {
    output << "void "_view << Perimortem::Abi::Core::object_retain_symbol
           << "(void *value);\nvoid "_view
           << Perimortem::Abi::Core::object_release_symbol
           << "(void *value);\n"_view;
  }

  for (const Tetrodotoxin::Terminal::Abi::Export& exported : exports) {
    if (!write_signature(
            output, types, names, exported.get_callable(),
            exported.get_symbol())) {
      return {};
    }
  }

  for (const Tetrodotoxin::Source::Reference<
           Tetrodotoxin::Library::Language::Foreign::State>& retained :
       foreign.get_states()) {
    const Tetrodotoxin::Library::Language::Foreign::State& addressable =
        retained.get();

    output << "extern "_view;
    if (addressable.get_definition().get_visibility() !=
        Tetrodotoxin::Language::Visibility::Public) {
      output << "const "_view;
    }

    auto type = require_addressable_type(addressable);
    if (!type || !write_type_name(output, types, names, *type)) {
      return {};
    }

    output << " "_view << addressable.get_name() << ";\n"_view;
  }

  for (const Tetrodotoxin::Source::Reference<
           Tetrodotoxin::Library::Language::Foreign::Function>& retained :
       foreign.get_functions()) {
    const Tetrodotoxin::Library::Language::Foreign::Function& callable =
        retained.get();
    if (!write_signature(
            output, types, names, callable, callable.get_symbol())) {
      return {};
    }
  }

  output << "\n#ifdef __cplusplus\n}\n#endif\n"_view;
  return Tetrodotoxin::Terminal::Abi::C::Header(buffer.get_view());
}

auto Tetrodotoxin::Terminal::Abi::C::Header::identify(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes source,
    Core::View::Bytes owner,
    Tetrodotoxin::Linker::Fingerprint fingerprint)
    -> Core::Option<Tetrodotoxin::Terminal::Abi::C::Header> {
  if (owner.is_empty() ||
      (!source.is_empty() &&
       (source.get_size() < header_opening.get_size() ||
        source.slice(0, header_opening.get_size()) != header_opening))) {
    return {};
  }

  // A Package containing only semantic or GPU members still owns one native
  // artifact agreement. Its minimal C header publishes that fingerprint while
  // an ordinary Library header contributes the declarations after the shared
  // preamble.
  Memory::Managed::Bytes buffer(arena);
  Stream::Textual<Memory::Managed::Bytes> output(buffer);
  output << header_opening << "#define TTX_ABI_FINGERPRINT_"_view;
  write_package_name(output, owner);
  output << " \""_view << fingerprint.render(arena) << "\"\n\n"_view;
  if (!source.is_empty()) {
    output << source.slice(header_opening.get_size());
  }
  return Tetrodotoxin::Terminal::Abi::C::Header(buffer.get_view());
}
