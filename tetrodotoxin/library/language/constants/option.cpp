// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/constants/option.hpp"

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

Constants::Option::Option(
    Memory::Allocator::Arena& domain,
    const Types::Option& type,
    Types::Option::Kind kind,
    Core::Option<Tetrodotoxin::Source::PackReference<Model::Pack>> payload,
    Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
    : Constant(anchor),
      type(type),
      kind(kind),
      payload(payload),
      name(
          domain,
          kind == Types::Option::Kind::Absent ? "none"_view : "some"_view) {
  if (kind == Types::Option::Kind::Present) {
    append_pack(name, payload->get());
  }
}

auto Constants::Option::create_absent(
    Memory::Allocator::Arena& domain,
    const Types::Option& type) -> Option& {
  return Constant::create_synthetic<Option>(domain, [&](auto source) -> Option {
    return Option(domain, type, Types::Option::Kind::Absent, {}, source);
  });
}

auto Constants::Option::create_present(
    Memory::Allocator::Arena& domain,
    const Types::Option& type,
    Model::Pack& payload) -> Core::Option<Option&> {
  BAIL_IF(!payload.fits_into(type.get_element_type()));
  const Layout& layout = payload.get_layout();
  for (Count index = 0; index < layout.get_size(); index++) {
    auto entry = layout.get_abstract(index);
    BAIL_IF(!entry || !entry->is<Constant>());
  }

  return Constant::create_synthetic<Option>(domain, [&](auto source) -> Option {
    return Option(
        domain, type, Types::Option::Kind::Present,
        Tetrodotoxin::Source::PackReference<Model::Pack>(payload), source);
  });
}

auto Constants::Option::create_fitted(
    Memory::Allocator::Arena& domain,
    const Types::Option& type,
    Model::Pack& source) -> Core::Option<Option&> {
  auto retained = source.select_identity<Constants::Option>();
  if (retained && &retained->get_type() == &type) {
    return *retained;
  }
  if (source.get_layout().is_empty()) {
    return create_absent(domain, type);
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

  retained = payload->select_identity<Constants::Option>();
  if (retained && &retained->get_type() == &type) {
    return *retained;
  }

  return create_present(domain, type, *payload);
}

auto Constants::Option::get_payload() const
    -> Core::Option<const Model::Pack&> {
  return payload.visit(
      []() -> Core::Option<const Model::Pack&> { return {}; },
      [](const Tetrodotoxin::Source::PackReference<Model::Pack>& selected)
          -> Core::Option<const Model::Pack&> { return selected.get(); });
}

auto Constants::Option::equals(const Constant& rhs) const -> Bool {
  auto selected = rhs.select<Constants::Option>();
  if (!selected || !has_same_type(rhs) || kind != selected->kind) {
    return False;
  }
  if (kind == Types::Option::Kind::Absent) {
    return True;
  }

  auto left_payload = get_payload();
  auto right_payload = selected->get_payload();
  return left_payload && right_payload &&
                 have_equal_values(*left_payload, *right_payload)
             ? True
             : False;
}
