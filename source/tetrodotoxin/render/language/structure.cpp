// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/structure.hpp"

#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Render;

auto Language::Structure::create(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition) -> Structure& {
  return domain.construct_from<Structure>(
      [&]() { return Structure(domain, definition); });
}

auto Language::Structure::retain_addressable(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  return declarations.retain_addressable(declaration, visibility);
}

auto Language::Structure::retain_callable(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  return declarations.retain_callable(declaration, visibility);
}

auto Language::Structure::retain_type(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  return declarations.retain_type(declaration, visibility);
}

auto Language::Structure::retain_instance(Tetrodotoxin::Source::Addressable& value)
    -> void {
  instances.insert(value);
}

auto Language::Structure::link(Cursor& cursor) -> Bool {
  return declarations.link(cursor, *this);
}

auto Language::Structure::link_restored() -> Bool {
  return declarations.link_restored(*this);
}

auto Language::Structure::resolve() const -> const Abstract& {
  return declarations.is_linked()
             ? static_cast<const Abstract&>(*this)
             : static_cast<const Abstract&>(Unknown::get_unknown());
}

auto Language::Structure::resolve_concept(View::Bytes name) const
    -> const Abstract& {
  if (name == "static"_view) {
    return declarations.get_authority();
  }
  if (name == "instance"_view) {
    return None::get_none();
  }
  const Abstract& local = declarations.resolve_type(
      name, Tetrodotoxin::Language::Visibility::Public);
  return local.is<Unknown>() || local.is<None>()
             ? definition.get_host().resolve_concept(name)
             : local;
}

auto Language::Structure::visit_concepts(
    Tetrodotoxin::Source::Abstract::Visitor visitor) const -> void {
  declarations.visit_concepts(visitor);
}

auto Language::Structure::resolve_local_context(View::Bytes name) const
    -> const Abstract& {
  return declarations.resolve_type(
      name, Tetrodotoxin::Language::Visibility::Private);
}

auto Language::Structure::get_layout() const -> const Tetrodotoxin::Source::Layout& {
  return layout;
}

auto Language::Structure::InstanceLayout::get_size() const -> Count {
  return owner.instances.get_size();
}

auto Language::Structure::InstanceLayout::get_abstract(Count index) const
    -> Option<const Abstract&> {
  BAIL_IF(index >= owner.instances.get_size());
  return owner.instances.at(index).get();
}

auto Language::Structure::InstanceLayout::get_name(Count index) const
    -> Option<View::Bytes> {
  BAIL_IF(index >= owner.instances.get_size());
  return owner.instances.at(index).get().get_name();
}

auto Language::Structure::InstanceLayout::fits_entry(
    const Tetrodotoxin::Source::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  auto source = get_abstract(source_index);
  auto destination = target.get_abstract(target_index);
  BAIL_IF(!source || !destination);
  auto source_value = source->select<Tetrodotoxin::Source::Addressable>();
  auto target_value = destination->select<Tetrodotoxin::Source::Addressable>();
  const Abstract& source_type =
      source_value ? static_cast<const Abstract&>(source_value->get_type())
                   : source->resolve();
  const Abstract& target_type =
      target_value ? static_cast<const Abstract&>(target_value->get_type())
                   : destination->resolve();
  return &source_type == &target_type;
}

auto Language::Structure::InstanceLayout::fits_at(
    const Tetrodotoxin::Source::Layout& target,
    Count target_offset) const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));
  for (Count index = 0; index < get_size(); index++) {
    BAIL_IF(!fits_entry(target, index, target_offset + index));
  }
  return True;
}

auto Language::Structure::InstanceLayout::get_fitted_at(
    const Tetrodotoxin::Source::Layout& target,
    Count target_offset,
    Count target_index) const -> Result<const Abstract&, Errors> {
  if (target_index >= get_size()) {
    return Errors::IndexOutOfBounds;
  }
  if (!has_target_segment(target, target_offset)) {
    return Errors::SizeMismatch;
  }
  if (!fits_entry(target, target_index, target_offset + target_index)) {
    return Errors::IncompatibleFit;
  }
  return *get_abstract(target_index);
}
