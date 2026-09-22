// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/abi/representation/type.hpp"

#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/value.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/implementation.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/object_storage.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/library/language/types/result.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library::Language;

auto Tetrodotoxin::Terminal::Abi::Representation::Type::get_kind(
    const Tetrodotoxin::Source::Type& type) -> Core::Option<Kind> {
  if (type.is<Model::Types::Value>()) {
    return Kind::Value;
  }
  if (type.is<Types::Enumeration>()) {
    return Kind::Enumeration;
  }
  if (type.is<Types::Fixed>()) {
    return Kind::Fixed;
  }
  if (type.is<Types::Option>()) {
    return Kind::Option;
  }
  if (type.is<Types::Result>()) {
    return Kind::Result;
  }
  if (type.is<Types::Range>()) {
    return Kind::Range;
  }
  if (type.is<Types::View>()) {
    return Kind::View;
  }
  if (type.is<Types::Access>()) {
    return Kind::Access;
  }
  if (type.is<Types::Implementation>()) {
    return Kind::Implementation;
  }
  if (type.is<Types::ObjectStorage>()) {
    return Kind::ObjectStorage;
  }
  if (type.is<Types::Object>()) {
    return Kind::Object;
  }
  if (type.is<Types::Source>()) {
    return Kind::Context;
  }
  auto structure = type.select<Types::Structure>();
  if (structure) {
    return structure->get_layout().is_empty() ? Kind::Context : Kind::Structure;
  }
  return {};
}

auto Tetrodotoxin::Terminal::Abi::Representation::Type::get_width(
    const Tetrodotoxin::Source::Type& type) -> Core::Option<Count> {
  auto value = type.select<Model::Types::Value>();
  return value ? Core::Option<Count>(value->get_width())
               : Core::Option<Count>();
}

auto Tetrodotoxin::Terminal::Abi::Representation::Type::get_element(
    const Tetrodotoxin::Source::Type& type) -> Core::Option<const Model::Type&> {
  auto enumeration = type.select<Types::Enumeration>();
  if (enumeration) {
    return enumeration->get_storage_type();
  }
  auto fixed = type.select<Types::Fixed>();
  if (fixed) {
    return fixed->get_element_type();
  }
  auto option = type.select<Types::Option>();
  if (option) {
    return option->get_element_type();
  }
  auto result = type.select<Types::Result>();
  if (result) {
    return result->get_value_type();
  }
  auto range = type.select<Types::Range>();
  if (range) {
    return range->get_element_type();
  }
  auto view = type.select<Types::View>();
  if (view) {
    return view->get_element_type();
  }
  auto access = type.select<Types::Access>();
  if (access) {
    return access->get_element_type();
  }
  auto storage = type.select<Types::ObjectStorage>();
  return storage ? Core::Option<const Model::Type&>(storage->get_element_type())
                 : Core::Option<const Model::Type&>();
}

auto Tetrodotoxin::Terminal::Abi::Representation::Type::get_flag(
    const Tetrodotoxin::Source::Type& type) -> Core::Option<const Model::Type&> {
  auto option = type.select<Types::Option>();
  if (option) {
    return option->get_flag_type();
  }
  auto result = type.select<Types::Result>();
  return result ? Core::Option<const Model::Type&>(result->get_flag_type())
                : Core::Option<const Model::Type&>();
}

auto Tetrodotoxin::Terminal::Abi::Representation::Type::get_error(
    const Tetrodotoxin::Source::Type& type) -> Core::Option<const Model::Type&> {
  auto result = type.select<Types::Result>();
  return result ? Core::Option<const Model::Type&>(result->get_error_type())
                : Core::Option<const Model::Type&>();
}

auto Tetrodotoxin::Terminal::Abi::Representation::Type::get_extent(
    const Tetrodotoxin::Source::Type& type) -> Core::Option<Count> {
  auto fixed = type.select<Types::Fixed>();
  return fixed ? Core::Option<Count>(fixed->get_extent())
               : Core::Option<Count>();
}

auto Tetrodotoxin::Terminal::Abi::Representation::Type::get_fields(
    const Tetrodotoxin::Source::Type& type) -> Core::Option<const Tetrodotoxin::Source::Layout&> {
  auto structure = type.select<Types::Structure>();
  return structure ? Core::Option<const Tetrodotoxin::Source::Layout&>(
                         structure->get_layout())
                   : Core::Option<const Tetrodotoxin::Source::Layout&>();
}

auto Tetrodotoxin::Terminal::Abi::Representation::Type::is_real(
    const Tetrodotoxin::Source::Type& type) -> Bool {
  return type.is<Model::Types::Real>();
}

auto Tetrodotoxin::Terminal::Abi::Representation::Type::is_signed(
    const Tetrodotoxin::Source::Type& type) -> Bool {
  return type.is<Model::Types::Signed>();
}

auto Tetrodotoxin::Terminal::Abi::Representation::Type::is_flag(
    const Tetrodotoxin::Source::Type& type) -> Bool {
  return type.is<Model::Types::Flag>();
}

auto Tetrodotoxin::Terminal::Abi::Representation::Type::is_object(
    const Tetrodotoxin::Source::Type& type) -> Bool {
  return type.is<Types::Object>();
}
