// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/result.hpp"

#include "tetrodotoxin/library/language/expression.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library::Language;

static auto append_pack(Memory::Managed::Bytes& output, const Model::Pack& pack)
    -> void {
  output.append('[');
  const Layout& layout = pack.get_layout();
  for (Count index = 0; index < layout.get_size(); index++) {
    if (index != 0) {
      output.concat(", "_view);
    }
    auto name = layout.get_name(index);
    if (name) {
      output.append('.');
      output.concat(*name);
      output.concat(" = "_view);
    }
    layout.get_abstract(index).visit(
        [&]() { output.concat("Unknown"_view); },
        [&](const Abstract& value) { output.concat(value.get_name()); });
  }
  output.append(']');
}

Constants::Result::Result(
    Memory::Allocator::Arena& domain,
    const Types::Result& type,
    Types::Result::Kind kind,
    Model::Pack& payload,
    Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
    : Constant(anchor),
      type(type),
      kind(kind),
      payload(payload),
      name(
          domain,
          kind == Types::Result::Kind::Value ? "value"_view : "error"_view) {
  append_pack(name, payload);
}

auto Constants::Result::create(
    Memory::Allocator::Arena& domain,
    const Types::Result& type,
    Types::Result::Kind kind,
    Model::Pack& payload) -> Core::Option<Result&> {
  const Model::Type& selected = kind == Types::Result::Kind::Value
                                    ? type.get_value_type()
                                    : type.get_error_type();
  BAIL_IF(!payload.fits_into(selected));
  const Layout& layout = payload.get_layout();
  for (Count index = 0; index < layout.get_size(); index++) {
    auto entry = layout.get_abstract(index);
    BAIL_IF(!entry || !entry->is<Constant>());
  }

  return Constant::create_synthetic<Result>(domain, [&](auto source) -> Result {
    return Result(domain, type, kind, payload, source);
  });
}

auto Constants::Result::create_value(
    Memory::Allocator::Arena& domain,
    const Types::Result& type,
    Model::Pack& payload) -> Core::Option<Result&> {
  return create(domain, type, Types::Result::Kind::Value, payload);
}

auto Constants::Result::create_error(
    Memory::Allocator::Arena& domain,
    const Types::Result& type,
    Model::Pack& payload) -> Core::Option<Result&> {
  return create(domain, type, Types::Result::Kind::Error, payload);
}

auto Constants::Result::select(Model::Pack& source) -> Core::Option<Result&> {
  auto direct = source.select_identity<Result>();
  if (direct) {
    return *direct;
  }

  const Layout& layout = source.get_layout();
  BAIL_IF(layout.get_size() != 1);
  return layout.get_abstract(0).visit(
      []() -> Core::Option<Result&> { return {}; },
      [](const Abstract& selected) -> Core::Option<Result&> {
        auto pack = Model::Pack::from(const_cast<Abstract&>(selected));
        return pack ? pack->select_identity<Result>() : Core::Option<Result&>();
      });
}

auto Constants::Result::create_fitted(
    Memory::Allocator::Arena& domain,
    const Types::Result& type,
    Model::Pack& source) -> Core::Option<Result&> {
  auto retained = select(source);
  if (retained && &retained->get_type() == &type) {
    return *retained;
  }

  Model::Pack* payload = &source;
  if (!source.select_identity<Constant>()) {
    Core::Option<Model::Pack&> folded;
    Expression::fold(source).visit(
        [&](const Core::Option<Model::Pack&>& selected) { folded = selected; },
        [](const Expression::Error&) {});
    BAIL_IF(!folded);
    payload = &*folded;
  }

  retained = select(*payload);
  if (retained && &retained->get_type() == &type) {
    return *retained;
  }

  Bool value = payload->fits_into(type.get_value_type());
  Bool error = payload->fits_into(type.get_error_type());
  BAIL_IF(value == error);
  return value ? create_value(domain, type, *payload)
               : create_error(domain, type, *payload);
}

auto Constants::Result::equals(const Constant& rhs) const -> Bool {
  auto selected = rhs.select<Constants::Result>();
  return selected && has_same_type(rhs) && kind == selected->kind &&
                 have_equal_values(payload.get(), selected->payload.get())
             ? True
             : False;
}
