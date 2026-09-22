// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/fixed.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/builtin/fixed/access.hpp"
#include "tetrodotoxin/library/builtin/fixed/view.hpp"
#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library::Language;

Types::Fixed::Fixed(
    Allocator::Arena& domain,
    View::Bytes name,
    const Model::Type& element,
    ::U64 extent,
    const Model::Type& access_type,
    const Model::Type& view_type)
    : name(name),
      element(element),
      extent(extent),
      layout(element, Count(extent)) {
  auto& get_access = Builtin::Fixed::Access::create(domain, *this, access_type);
  auto& get_view = Builtin::Fixed::View::create(domain, *this, view_type);
  publish_callable(domain, get_access, True);
  publish_callable(domain, get_view, True);
}

auto Types::Fixed::create_default(Allocator::Arena& arena) const
    -> Option<Model::Pack&> {
  BAIL_IF(get_extent() == 0 || get_extent() > U64(Count(-1)));

  Managed::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>> values(arena);
  values.reset(Count(get_extent()));
  for (Count index = 0; index < Count(get_extent()); index++) {
    auto value = get_element_type().create_default(arena);
    BAIL_IF(!value);
    values.insert(*value);
  }

  return Expressions::Initializer::create_synthetic(
      arena, *this, values.get_view());
}

static auto fold_output(Model::Pack& source, Count index)
    -> Option<Model::Pack&> {
  auto producer = source.get_layout().get_abstract(index);
  BAIL_IF(!producer);
  auto direct = const_cast<Abstract&>(*producer)
                    .select<Tetrodotoxin::Library::Language::Constant>();
  if (direct) {
    return *direct;
  }

  auto expression = const_cast<Abstract&>(*producer).select<Expression>();
  BAIL_IF(!expression);

  Option<Model::Pack&> folded;
  expression->fold().visit(
      [&](const Option<Model::Pack&>& selected) { folded = selected; },
      [](const Expression::Error&) {});
  BAIL_IF(!folded);

  Count selected_index = 0;
  for (Count source_index = 0; source_index < index; source_index++) {
    auto prior = source.get_layout().get_abstract(source_index);
    selected_index += prior && &*prior == &*producer ? 1 : 0;
  }
  auto selected = folded->get_layout().get_abstract(selected_index);
  BAIL_IF(!selected);
  auto constant = selected->select<Tetrodotoxin::Library::Language::Constant>();
  BAIL_IF(!constant);
  return const_cast<Tetrodotoxin::Library::Language::Constant&>(*constant);
}

static auto create_bytes(
    Allocator::Arena& arena,
    const Types::Fixed& type,
    View::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>> values)
    -> Option<Model::Pack&> {
  auto element =
      type.get_element_type().resolve().select<Model::Types::Unsigned>();
  BAIL_IF(!element || element->get_size() != 1);

  auto storage = arena.allocate(values.get_size());
  Count index = 0;
  for (const Tetrodotoxin::Source::PackReference<Model::Pack>& selected : values) {
    auto value = selected.get().select_identity<Constants::Unsigned>();
    BAIL_IF(!value || value->get_value() > U64(U8(-1)));
    storage.get_data()[index] = U8(value->get_value());
    index++;
  }

  return Constants::Bytes::create_synthetic(
      arena, type, View::Bytes(storage.get_data(), storage.get_size()));
}

auto Types::Fixed::create_fitted(Allocator::Arena& arena, Model::Pack& source)
    const -> Option<Model::Pack&> {
  BAIL_IF(!source.fits(*this));

  Managed::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>> values(arena);
  values.reset(Count(get_extent()));
  for (Count index = 0; index < Count(get_extent()); index++) {
    auto value = fold_output(source, index);
    BAIL_IF(!value);
    values.insert(*value);
  }

  auto element = get_element_type().resolve().select<Model::Types::Unsigned>();
  if (element && element->get_size() == 1) {
    return create_bytes(arena, *this, values.get_view());
  }
  return Model::Pack::create_folded(arena, values.get_view());
}
