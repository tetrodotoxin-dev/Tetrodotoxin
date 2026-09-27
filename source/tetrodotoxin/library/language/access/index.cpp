// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/index.hpp"

#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;

static auto select_access(const Abstract& output)
    -> Core::Option<const Language::Types::Access&> {
  auto direct = output.select<Language::Types::Access>();
  if (direct) {
    return direct;
  }

  return output.resolve().select<Language::Types::Access>();
}

static auto is_integer(const Language::Model::Pack& pack) -> Bool {
  const Abstract& output = pack.get_type();
  auto direct = output.select<Language::Model::Type>();
  const Abstract& type =
      direct ? static_cast<const Abstract&>(*direct) : output.resolve();
  return type.is<Tetrodotoxin::Library::Language::Model::Types::Signed>() ||
         type.is<Tetrodotoxin::Library::Language::Model::Types::Unsigned>();
}

static auto get_range_count(Language::Model::Pack& pack)
    -> Core::Option<Count> {
  Core::Option<Language::Model::Pack&> folded;
  Core::Option<Language::Expression::Error> fold_error;
  Language::Expression::fold(pack).visit(
      [&](const Core::Option<Language::Model::Pack&>& selected) {
        folded = selected;
      },
      [&](const Language::Expression::Error& error) { fold_error = error; });
  BAIL_IF(fold_error || !folded);

  auto scalar = folded->select_identity<Language::Constant>();
  BAIL_IF(!scalar);
  return scalar->visit<Language::Constants::Signed>(
      [](const Language::Constants::Signed& value) -> Core::Option<Count> {
        BAIL_IF(value.get_value() < 0);
        U64 selected = U64(value.get_value());
        BAIL_IF(selected > U64(Count(-1)));
        return Count(selected);
      },
      [](const Abstract& selected) -> Core::Option<Count> {
        auto value = selected.select<Language::Constants::Unsigned>();
        BAIL_IF(!value || value->get_value() > U64(Count(-1)));
        return Count(value->get_value());
      });
}

auto Language::Access::Index::create_authored(
    Memory::Allocator::Arena& domain,
    Model::Pack& receiver,
    Model::Pack& index,
    Anchor anchor) -> Index& {
  return Expression::create_authored<Index>(
      domain, anchor,
      [&](auto authored) -> Index { return Index(receiver, index, authored); });
}

auto Language::Access::Index::create_authored(
    Memory::Allocator::Arena& domain,
    Model::Pack& receiver,
    Model::Pack& start,
    Model::Pack& count,
    Anchor anchor) -> Index& {
  return Expression::create_authored<Index>(
      domain, anchor, [&](auto authored) -> Index {
        return Index(receiver, start, count, authored);
      });
}

auto Language::Access::Index::link_target(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(cursor, lexical_context, access_scope));
  BAIL_IF(!first.link(cursor, lexical_context, access_scope));
  if (count) {
    BAIL_IF(!count->get().link(cursor, lexical_context, access_scope));
  }

  // Index links both Expressions before reading their output domains. The
  // element Type is reference metadata only, so this owner never performs
  // bounds checks, default selection, or ordinary value materialization.
  auto access = select_access(receiver.get_type());
  if (!access || !is_integer(first) || (count && !is_integer(count->get()))) {
    cursor.create_expression_error(
        get_anchor(),
        "Index access rejects the linked receiver or operand Types."_view,
        "Use Access[T] with signed or unsigned index, start, and count Expressions."_view);
    return False;
  }

  const Language::Model::Type& selected = access->get_element_type();
  if (element_type && &element_type->get() != &selected) {
    cursor.create_expression_error(
        get_anchor(),
        "Index access cannot change its referenced element Type."_view,
        "Keep one exact Access element Type bound to this authored index."_view);
    return False;
  }

  element_type = Reference<const Language::Model::Type>(selected);

  if (count) {
    auto selected_count = ::get_range_count(count->get());
    if (!selected_count) {
      cursor.create_expression_error(
          count->get().get_anchor(),
          "Index range count did not fold to a supported nonnegative integer."_view,
          "Use one integer Constant representable as Count."_view);
      return False;
    }

    if (range_count && *range_count != *selected_count) {
      cursor.create_expression_error(
          get_anchor(), "Index cannot change its linked range count."_view,
          "Keep one exact constant count for this authored target."_view);
      return False;
    }
    range_count = *selected_count;
  }

  return True;
}

auto Language::Access::Index::link(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(!link_target(cursor, lexical_context, access_scope));

  cursor.create_expression_error(
      get_anchor(),
      "Index is a write selector and does not produce a value."_view,
      "Use `:[...]` for safe value selection or assign through this target."_view);
  return False;
}

auto Language::Access::Index::finalize(Cursor& cursor) -> void {
  receiver.finalize(cursor);
  first.finalize(cursor);
  if (count) {
    count->get().finalize(cursor);
  }
}

auto Language::Access::Index::get_element_type() const -> const Abstract& {
  return element_type.visit(
      []() -> const Abstract& { return Unknown::get_unknown(); },
      [](const Reference<const Language::Model::Type>& selected)
          -> const Abstract& { return selected.get(); });
}

auto Language::Access::Index::get_type() const -> const Abstract& {
  return count ? static_cast<const Abstract&>(Unknown::get_unknown())
               : get_element_type();
}

auto Language::Access::Index::get_write_type(const Language::Model::Type&) const
    -> Core::Option<const Language::Model::Type&> {
  BAIL_IF(count);
  return element_type.visit(
      []() -> Core::Option<const Language::Model::Type&> { return {}; },
      [](const Reference<const Language::Model::Type>& selected)
          -> Core::Option<const Language::Model::Type&> {
        return selected.get();
      });
}

auto Language::Access::Index::resolve() const -> const Abstract& {
  return Unknown::get_unknown();
}

auto Language::Access::Index::link_write_target(
    Cursor& cursor,
    const Abstract& lexical_context,
    const Language::Model::Type& access_scope) -> Bool {
  return link_target(cursor, lexical_context, access_scope);
}

auto Language::Access::Index::accepts_write(
    const Language::Model::Pack& source,
    const Language::Model::Type&) const -> Bool {
  BAIL_IF(!element_type);
  const Language::Model::Type& element = element_type->get();
  if (!count) {
    return source.fits_into(element);
  }

  BAIL_IF(!range_count || source.get_layout().get_size() != *range_count);
  for (Count index = 0; index < *range_count; index++) {
    BAIL_IF(!source.fits_entry(element.get_layout(), index, 0));
  }
  return True;
}
