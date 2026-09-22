// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/layout.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/layouts/addressable.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

static auto select_entry_type(const Abstract& entry) -> Option<const Type&> {
  auto type = entry.select<Type>();
  if (type) {
    return *type;
  }

  return entry.select<Addressable>().visit(
      []() -> Option<const Type&> { return {}; },
      [](const Addressable& addressable) -> Option<const Type&> {
        return addressable.get_type().select<Type>();
      });
}

static auto resolves_for_fitting(const Abstract& entry) -> const Abstract& {
  // Authored Layouts retain direct Type and Parameter edges before every Type
  // necessarily resolves. Those identities are already canonical. Resolving
  // first would collapse distinct staged Types to the shared Unknown object.
  if (entry.is<Type>()) {
    return entry;
  }

  auto direct_addressable = entry.select<Addressable>();
  if (direct_addressable) {
    return direct_addressable->get_type();
  }

  const Abstract& represented = entry.resolve();
  return represented.visit<Addressable>(
      [](const Addressable& addressable) -> const Abstract& {
        return addressable.get_type();
      },
      [](const Abstract& abstract) -> const Abstract& { return abstract; });
}

static auto get_slot_name(const Tetrodotoxin::Source::Layout& layout, Count index)
    -> Option<View::Bytes> {
  auto explicit_name = layout.get_name(index);
  if (explicit_name) {
    return explicit_name;
  }

  return layout.get_abstract(index).visit(
      []() -> Option<View::Bytes> { return {}; },
      [](const Abstract& selected) -> Option<View::Bytes> {
        View::Bytes name = selected.get_name();
        return name.is_empty() ? Option<View::Bytes>()
                               : Option<View::Bytes>(name);
      });
}

auto Language::Model::Layout::create_authored(
    Allocator::Arena& domain,
    Managed::Vector<Slot> slots,
    Anchor anchor,
    Bool parameters) -> Layout& {
  return create(domain, slots, anchor, parameters);
}

auto Language::Model::Layout::create(
    Allocator::Arena& domain,
    Managed::Vector<Slot> slots,
    Anchor anchor,
    Bool parameters) -> Layout& {
  return domain.construct_from<Layout>(
      [&]() -> Layout { return Layout(domain, slots, anchor, parameters); });
}

auto Language::Model::Layout::retain_generated_slot(
    TypeReference type_reference,
    View::Bytes name,
    View::Vector<Tetrodotoxin::Language::Attribute> attributes) -> Bool {
  BAIL_IF(!slots.is_empty() && is_linked());
  for (const Slot& slot : slots.get_view()) {
    BAIL_IF(!name.is_empty() && slot.get_name() == name);
  }

  slots.insert(Slot(type_reference, Anchor::create(Span()), name, attributes));
  return True;
}

auto Language::Model::Layout::retain_generated_slot(
    const Type& type,
    View::Bytes name,
    View::Vector<Tetrodotoxin::Language::Attribute> attributes) -> Bool {
  BAIL_IF(type.get_layout().is_empty());
  for (const Slot& slot : slots.get_view()) {
    BAIL_IF(!name.is_empty() && slot.get_name() == name);
  }

  Slot slot({}, Anchor::create(Span()), name, attributes);
  if (parameters) {
    auto parameter =
        Tetrodotoxin::Source::Layouts::Addressable::create_authored(domain, name, type);
    BAIL_IF(!parameter);
    slot.edge = Reference<const Abstract>(*parameter);
  } else {
    slot.edge = Reference<const Abstract>(type);
  }
  slots.insert(slot);
  return True;
}

auto Language::Model::Layout::retain_generated_edge(
    Count index,
    const Type& type) -> Bool {
  BAIL_IF(
      index >= slots.get_size() || slots[index].type_reference ||
      type.get_layout().is_empty());
  Slot& slot = slots[index];
  if (slot.edge) {
    auto retained = select_entry_type(slot.edge->get());
    return retained && &*retained == &type;
  }
  if (parameters) {
    auto parameter = Tetrodotoxin::Source::Layouts::Addressable::create_authored(
        domain, slot.name, type);
    BAIL_IF(!parameter);
    slot.edge = Reference<const Abstract>(*parameter);
  } else {
    slot.edge = Reference<const Abstract>(type);
  }
  return True;
}

auto Language::Model::Layout::link_restored(
    const Abstract& host,
    Bool parameters,
    Option<const Tetrodotoxin::Source::Addressable&> self) -> Bool {
  if (is_linked()) {
    return True;
  }

  for (Count index = 0; index < slots.get_size(); index++) {
    Slot& slot = slots[index];
    if (!slot.type_reference && slot.edge) {
      auto retained = select_entry_type(slot.edge->get());
      BAIL_IF(!retained || retained->get_layout().is_empty());
      continue;
    }
    Option<const Type&> type;
    if (!slot.type_reference) {
      if (!parameters) {
        BAIL_IF(
            index != 0 || slots.get_size() != 1 || slot.name != "self"_view ||
            !self);
        if (slot.edge) {
          BAIL_IF(&slot.edge->get() != &*self);
        } else {
          slot.edge = Reference<const Abstract>(*self);
        }
        continue;
      }

      auto host_type = host.select<Type>();
      BAIL_IF(index != 0 || slot.name != "self"_view || !host_type);
      type = *host_type;
    } else {
      slot.type_reference->resolve_lexical(host).visit(
          [&](const Abstract& selected) { type = selected.select<Type>(); },
          [](const TypeReference::Failure&) {});
      BAIL_IF(!type);
    }

    BAIL_IF(type->get_layout().is_empty());
    if (parameters) {
      auto source = slot.type_reference ? slot.type_reference->get_interface()
                                        : type->get_interface();
      auto parameter = Tetrodotoxin::Source::Layouts::Addressable::create_authored(
          domain, slot.name, *type, source);
      BAIL_IF(!parameter);
      slot.edge = Reference<const Abstract>(*parameter);
    } else {
      slot.edge = Reference<const Abstract>(*type);
    }
  }

  BAIL_IF(is_named() && !has_unique_names());
  return True;
}

auto Language::Model::Layout::link_parameters(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Abstract& host) -> Bool {
  return link(cursor, host, True, {});
}

auto Language::Model::Layout::link_types(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Abstract& host,
    Option<const Tetrodotoxin::Source::Addressable&> self) -> Bool {
  return link(cursor, host, False, self);
}

auto Language::Model::Layout::link(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Abstract& host,
    Bool parameters,
    Option<const Tetrodotoxin::Source::Addressable&> self) -> Bool {
  // Linking settles every slot before exposing the Layout. Parameter Layouts
  // replace each authored slot with one real Layout-owned Addressable. Ordinary
  // results retain their selected Type, while `self` reuses parameter zero.
  Bool failed = False;
  for (Count i = 0; i < slots.get_size(); i++) {
    Slot& slot = slots[i];
    if (!slot.type_reference && slot.edge) {
      auto retained = select_entry_type(slot.edge->get());
      if (!retained || retained->get_layout().is_empty()) {
        cursor.create_expression_error(
            slot.anchor,
            "Generated Function Layout edge no longer selects a value Type."_view);
        failed = True;
      }
      continue;
    }
    Option<const Type&> type;
    if (!slot.type_reference) {
      if (!parameters) {
        if (i != 0 || slots.get_size() != 1 || slot.name != "self"_view ||
            !self) {
          cursor.create_expression_error(
              slot.anchor,
              "Function result `[self]` requires one Self Callable receiver."_view,
              "Declare `self` as parameter entry zero or return an authored Type."_view);
          failed = True;
          continue;
        }

        if (slot.edge && &slot.edge->get() != &*self) {
          cursor.create_expression_error(
              slot.anchor,
              "Repeated `[self]` result linking selected a different receiver."_view,
              "Preserve the Function's original self Addressable identity."_view);
          failed = True;
        } else if (!slot.edge) {
          slot.edge = Reference<const Abstract>(*self);
        }
        continue;
      }

      // Self is derived from the exact host because its reserved spelling is a
      // receiver role rather than a route that another context may intercept.
      if (i != 0 || slot.name != "self"_view) {
        cursor.create_expression_error(
            slot.anchor,
            "Only a leading Function parameter may derive its Type from "
            "`self`."_view,
            "Use an authored Type reference for every other Layout entry."_view);
        failed = True;
        continue;
      }
      auto host_type = host.select<Type>();
      if (!host_type) {
        cursor.create_expression_error(
            slot.anchor, "Library `self` requires one exact host Type."_view);
        failed = True;
        continue;
      }
      type = *host_type;
    } else {
      auto selected = slot.type_reference->resolve_authored(cursor, host);
      if (!selected) {
        failed = True;
        continue;
      }
      auto selected_type = selected->select<Type>();
      if (!selected_type) {
        cursor.create_expression_error(
            slot.get_type_anchor(),
            "Library Layout Type route did not resolve to one stable Type."_view,
            "Publish the named Type in this logical context before linking."_view);
        failed = True;
        continue;
      }
      type = *selected_type;
    }

    if (type->get_layout().is_empty()) {
      cursor.create_expression_error(
          slot.get_type_anchor(),
          parameters
              ? "Function parameter cannot bind an empty Type Layout."_view
              : "Function result cannot name an empty Type Layout."_view,
          parameters
              ? "Remove the parameter or use a Type with one value leaf."_view
              : "Write `[]` when the Function produces no values."_view);
      failed = True;
      continue;
    }

    if (!parameters) {
      // Repeated phase entry may observe the same identity but must never move
      // an already published slot to a newly selected Type.
      if (slot.edge) {
        if (&slot.edge->get() != &*type) {
          cursor.create_expression_error(
              slot.get_type_anchor(),
              "Repeated Layout linking selected a different Type identity."_view,
              "Preserve the original resolved Type edge across completion."_view);
          failed = True;
        }
      } else {
        slot.edge = Reference<const Abstract>(*type);
      }
      continue;
    }

    if (slot.edge) {
      auto parameter =
          slot.edge->get().select<Tetrodotoxin::Source::Layouts::Addressable>();
      if (!parameter || &parameter->get_type() != &*type) {
        cursor.create_expression_error(
            slot.get_type_anchor(),
            "Repeated parameter linking selected a different semantic edge."_view,
            "Preserve the original parameter Addressable and Type identity."_view);
        failed = True;
      }
      continue;
    }

    auto source = slot.type_reference ? slot.type_reference->get_interface()
                                      : type->get_interface();
    auto parameter = Tetrodotoxin::Source::Layouts::Addressable::create_authored(
        domain, slot.name, *type, source);
    if (!parameter) {
      cursor.create_expression_error(
          slot.anchor,
          "Function parameter could not retain its authored Layout entry."_view,
          "Use one named nonempty Type for each Function parameter."_view);
      failed = True;
      continue;
    }
    slot.edge = Reference<const Abstract>(*parameter);
    cursor.get_associations().create(slot.anchor, *parameter);
  }

  return !failed && is_linked();
}

auto Language::Model::Layout::resolve_named(
    View::Bytes route,
    Option<const Abstract&> host) const -> const Abstract& {
  for (Count i = 0; i < get_size(); i++) {
    Slot& slot = slots.at(i);
    if (slot.name != route) {
      continue;
    }
    if (slot.edge) {
      return slot.edge->get();
    }
    if (!parameters || !host || !slot.type_reference) {
      return Unknown::get_unknown();
    }

    Option<const Type&> type;
    slot.type_reference->resolve_lexical(*host).visit(
        [&](const Abstract& selected) { type = selected.select<Type>(); },
        [](const TypeReference::Failure&) {});
    if (!type || type->get_layout().is_empty()) {
      return Unknown::get_unknown();
    }
    auto source = slot.type_reference ? slot.type_reference->get_interface()
                                      : type->get_interface();
    auto parameter = Tetrodotoxin::Source::Layouts::Addressable::create_authored(
        domain, slot.name, *type, source);
    if (!parameter) {
      return Unknown::get_unknown();
    }
    slot.edge = Reference<const Abstract>(*parameter);
    return *parameter;
  }

  return Unknown::get_unknown();
}

auto Language::Model::Layout::get_slot_attributes(Count index) const
    -> View::Vector<Tetrodotoxin::Language::Attribute> {
  return index < slots.get_size()
             ? slots.at(index).get_attributes()
             : View::Vector<Tetrodotoxin::Language::Attribute>();
}

auto Language::Model::Layout::validate_publication(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Abstract& host) const -> Bool {
  auto context = host.select<Language::Model::Type>();
  if (!is_linked() || !context) {
    cursor.create_expression_error(
        anchor,
        "A published Function requires one linked Library Type context."_view,
        "Link every authored Layout entry on its exact Function host before "
        "publication."_view);
    return False;
  }

  Bool valid = True;
  for (Count i = 0; i < slots.get_size(); i++) {
    const Slot& slot = slots.at(i);
    auto type = select_entry_type(slot.edge->get());
    if (!type) {
      cursor.create_expression_error(
          slot.get_type_anchor(),
          "A published Function Layout retains an invalid semantic entry."_view,
          "Retain the exact Addressable or Type selected during linking."_view);
      valid = False;
      continue;
    }

    // Publication repeats the authored query through the host's public graph.
    // A private Type remains usable locally but cannot leak through a readable
    // Function signature merely because linking retained its identity.
    Bool reachable = slot.type_reference.visit(
        [&]() {
          // A generated slot already retains the exact Type selected by its
          // embedding Dialect. Authored slots still repeat their public route
          // below, while self proves the same direct host relationship.
          return Bool(
              slot.edge &&
              ((i == 0 && slot.name == "self"_view && &*type == &host) ||
               slot.name != "self"_view));
        },
        [&](const TypeReference& reference) {
          Option<const Abstract&> selected;
          reference.resolve(*context).visit(
              [&](const Abstract& resolved) { selected = resolved; },
              [](const TypeReference::Failure&) {});
          return Bool(selected && &selected->resolve() == &*type);
        });
    if (!reachable) {
      cursor.create_expression_error(
          slot.get_type_anchor(),
          "Externally readable Function publishes an unreachable Type "
          "route."_view,
          "Keep the Function private or publish its authored Type route."_view);
      valid = False;
    }
  }

  return valid;
}

auto Language::Model::Layout::declares_self() const -> Bool {
  if (slots.is_empty()) {
    return False;
  }

  const Slot& first = slots.at(0);
  return Bool(!first.type_reference && first.name == "self"_view);
}

auto Language::Model::Layout::is_linked() const -> Bool {
  for (Count i = 0; i < slots.get_size(); i++) {
    if (!slots.at(i).edge) {
      return False;
    }
  }
  return True;
}

auto Language::Model::Layout::is_named() const -> Bool {
  if (!parameters && slots.get_size() == 1 && slots.at(0).name == "self"_view &&
      !slots.at(0).type_reference) {
    return False;
  }
  return slots.is_empty() || !slots.at(0).name.is_empty();
}

auto Language::Model::Layout::get_slot(Count index) const
    -> Option<const Slot&> {
  return index < slots.get_size() ? Option<const Slot&>(slots.at(index))
                                  : Option<const Slot&>();
}

auto Language::Model::Layout::get_size() const -> Count {
  return slots.get_size();
}

auto Language::Model::Layout::get_interface() const
    -> Tetrodotoxin::Source::Layout::Handle {
  static const Tetrodotoxin::Source::Layout::Operations operations = {
    [](const void* source) -> Count {
      return static_cast<const Layout*>(source)->get_size();
    },
    [](const void* source, Count index) -> Option<Abstract::Handle> {
      const auto& layout = *static_cast<const Layout*>(source);
      auto slot = layout.get_slot(index);
      if (!slot || !slot->edge) {
        return {};
      }
      if (!layout.parameters && slot->type_reference) {
        return slot->type_reference->get_interface();
      }
      return slot->edge->get().get_interface();
    },
    [](const void* source, Count index) -> Option<View::Bytes> {
      return static_cast<const Layout*>(source)->get_name(index);
    },
  };
  return Tetrodotoxin::Source::Layout::Handle(this, operations);
}

auto Language::Model::Layout::get_abstract(Count index) const
    -> Option<const Abstract&> {
  auto slot = get_slot(index);
  BAIL_IF(!slot || !slot->edge);
  return slot->edge->get();
}

auto Language::Model::Layout::get_name(Count index) const
    -> Option<View::Bytes> {
  BAIL_IF(!is_named());

  auto slot = get_slot(index);
  BAIL_IF(!slot || slot->name.is_empty());
  return slot->name;
}

auto Language::Model::Layout::get_slot_anchor(Count index) const
    -> Option<Tetrodotoxin::Source::Lexical::Anchor> {
  auto slot = get_slot(index);
  BAIL_IF(!slot);
  return slot->anchor;
}

auto Language::Model::Layout::get_type_reference(Count index) const
    -> Option<const TypeReference&> {
  auto slot = get_slot(index);
  BAIL_IF(!slot || !slot->type_reference);
  return *slot->type_reference;
}

auto Language::Model::Layout::get_declared_name(Count index) const
    -> View::Bytes {
  return index < slots.get_size() ? slots.at(index).name : View::Bytes();
}

auto Language::Model::Layout::fits_value(
    const Tetrodotoxin::Source::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  auto source = get_abstract(source_index);
  auto destination = target.get_abstract(target_index);
  return Bool(
      source && destination &&
      &resolves_for_fitting(*source) == &resolves_for_fitting(*destination));
}

auto Language::Model::Layout::fits_entry(
    const Tetrodotoxin::Source::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  BAIL_IF(source_index >= get_size() || target_index >= target.get_size());

  if (!is_named()) {
    return fits_value(target, source_index, target_index);
  }

  auto source_name = get_name(source_index);
  auto target_name = get_slot_name(target, target_index);
  return source_name && target_name && *source_name == *target_name &&
         fits_value(target, source_index, target_index);
}

auto Language::Model::Layout::has_unique_names() const -> Bool {
  for (Count i = 0; i < get_size(); i++) {
    auto name = get_name(i);
    BAIL_IF(!name);
    for (Count other = i + 1; other < get_size(); other++) {
      auto candidate = get_name(other);
      BAIL_IF(!candidate || *candidate == *name);
    }
  }
  return True;
}

auto Language::Model::Layout::fits_at(
    const Tetrodotoxin::Source::Layout& target,
    Count target_offset) const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));

  if (!is_named()) {
    // Positional Layouts preserve authored order. Named Layouts instead match
    // within one equal sized target segment so names may reorder without
    // reaching outside the receiving declaration's boundary.
    for (Count i = 0; i < get_size(); i++) {
      BAIL_IF(!fits_value(target, i, target_offset + i));
    }
    return True;
  }

  BAIL_IF(!has_unique_names());
  for (Count source_index = 0; source_index < get_size(); source_index++) {
    auto source_name = get_name(source_index);
    BAIL_IF(!source_name);

    Count selected = 0;
    Count matches = 0;
    for (Count target_index = 0; target_index < get_size(); target_index++) {
      auto target_name = get_slot_name(target, target_offset + target_index);
      if (target_name && *target_name == *source_name) {
        selected = target_index;
        matches++;
      }
    }
    BAIL_IF(
        matches != 1 ||
        !fits_value(target, source_index, target_offset + selected));
  }

  return True;
}

auto Language::Model::Layout::get_fitted_at(
    const Tetrodotoxin::Source::Layout& target,
    Count target_offset,
    Count target_index) const -> Result<const Abstract&, Errors> {
  if (target_index >= get_size()) {
    return Errors::IndexOutOfBounds;
  }
  if (!has_target_segment(target, target_offset)) {
    return Errors::SizeMismatch;
  }
  if (!fits_at(target, target_offset)) {
    return Errors::IncompatibleFit;
  }

  if (!is_named()) {
    return get_abstract(target_index)
        .visit(
            []() -> Result<const Abstract&, Errors> {
              return Errors::IncompatibleFit;
            },
            [](const Abstract& selected) -> Result<const Abstract&, Errors> {
              return selected;
            });
  }

  auto target_name = get_slot_name(target, target_offset + target_index);
  if (!target_name) {
    return Errors::IncompatibleFit;
  }

  for (Count source_index = 0; source_index < get_size(); source_index++) {
    auto source_name = get_name(source_index);
    if (source_name && *source_name == *target_name) {
      return get_abstract(source_index)
          .visit(
              []() -> Result<const Abstract&, Errors> {
                return Errors::IncompatibleFit;
              },
              [](const Abstract& selected) -> Result<const Abstract&, Errors> {
                return selected;
              });
    }
  }

  return Errors::IncompatibleFit;
}
