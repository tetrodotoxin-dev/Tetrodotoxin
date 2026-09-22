// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/attribute.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/source/layout.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Layout is one authored Library descriptor and its eventual TTX Layout. Each
// source slot retains its TypeReference and exactly one delayed semantic edge.
// Parameter linking installs a real Layout-owned Addressable, while ordinary
// descriptor linking installs the exact Type. There is no parallel resolved
// inventory or optional semantic Layout to drift from those canonical slots.
//
// A nonempty authored Layout becomes observable through the TTX Layout
// contract only after every slot links. Empty `[]` is complete immediately.
// Function resolution enforces that boundary. Registration before link derives
// receiver role from declares_self() without pretending an unresolved shape is
// empty value flow.
class Layout final : public Tetrodotoxin::Source::Layout {
 public:
  // Slot is the retained source description for one Layout entry. Its Type
  // route and Anchor are model facts while interpretation alone decides how
  // punctuation produces this value.
  class Slot {
   public:
    constexpr Slot(
        Perimortem::Core::Option<TypeReference> type_reference,
        Tetrodotoxin::Source::Lexical::Anchor anchor,
        Perimortem::Core::View::Bytes name,
        Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
            attributes = {})
        : type_reference(type_reference),
          anchor(anchor),
          name(name),
          attributes(attributes) {}

    constexpr auto get_type_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor {
      return type_reference.visit(
          [&]() { return anchor; },
          [](const TypeReference& reference) {
            return reference.get_anchor();
          });
    }

    constexpr auto has_type_reference() const -> Bool {
      return Bool(type_reference);
    }

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }

    // Attributes remain uninterpreted source facts on the exact Layout slot.
    // Library execution ignores keys it does not own, while an embedding
    // Dialect can use the same slot to express a richer interface contract.
    constexpr auto get_attributes() const
        -> Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute> {
      return attributes;
    }

   private:
    friend class Layout;

    Perimortem::Core::Option<TypeReference> type_reference;
    Tetrodotoxin::Source::Lexical::Anchor anchor;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
        attributes;
    Perimortem::Core::Option<
        Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>>
        edge;
  };

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Memory::Managed::Vector<Slot> slots,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Bool parameters) -> Layout&;

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Memory::Managed::Vector<Slot> slots,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Bool parameters) -> Layout&;

  // Shader inherits Stage signatures after neighboring Render contracts have
  // composed. The real Function already owns this Layout, so adding generated
  // slots here completes that one descriptor without retaining parser state or
  // creating a second signature model.
  auto retain_generated_slot(
      TypeReference type_reference,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
          attributes = {}) -> Bool;

  auto retain_generated_slot(
      const Type& type,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
          attributes = {}) -> Bool;

  auto retain_generated_edge(Count index, const Type& type) -> Bool;

  auto link_restored(
      const Tetrodotoxin::Source::Abstract& host,
      Bool parameters,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Addressable&> self = {})
      -> Bool;

  Layout(const Layout&) = delete;
  Layout(Layout&&) = delete;
  auto operator=(const Layout&) -> Layout& = delete;
  auto operator=(Layout&&) -> Layout& = delete;

  // Both paths resolve the same authored TypeReference facts. Parameter slots
  // materialize real Layout-owned Addressables, results retain selected Types,
  // and the reserved scalar result `self` retains parameter entry zero itself.
  // Every authored Type slot must provide a value. Only `[]` carries an empty
  // descriptor.
  auto link_parameters(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& host) -> Bool;

  auto link_types(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& host,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Addressable&> self = {})
      -> Bool;

  // Named lookup returns the exact semantic entry retained by this Layout.
  // Positional, incomplete, or missing selections resolve Unknown.
  auto resolve_named(
      Perimortem::Core::View::Bytes route,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> host = {}) const
      -> const Tetrodotoxin::Source::Abstract&;

  // Publication remains beside the authored routes and final edges it checks.
  auto validate_publication(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& host) const -> Bool;

  auto declares_self() const -> Bool;
  auto is_linked() const -> Bool;

  auto get_size() const -> Count override;

  auto get_interface() const -> Tetrodotoxin::Source::Layout::Handle override;

  auto get_abstract(Count index) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> override;

  auto get_name(Count index) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> override;

  auto get_slot_anchor(Count index) const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor>;

  auto get_slot_attributes(Count index) const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>;

  auto get_type_reference(Count index) const
      -> Perimortem::Core::Option<const TypeReference&>;

  auto get_declared_name(Count index) const -> Perimortem::Core::View::Bytes;

  auto fits_entry(
      const Tetrodotoxin::Source::Layout& target,
      Count source_index,
      Count target_index) const -> Bool override;

  auto fits_at(const Tetrodotoxin::Source::Layout& target, Count target_offset) const
      -> Bool override;

  auto get_fitted_at(
      const Tetrodotoxin::Source::Layout& target,
      Count target_offset,
      Count target_index) const
      -> Perimortem::Utility::Result<
          const Tetrodotoxin::Source::Abstract&,
          Tetrodotoxin::Source::Layout::Errors> override;

 private:
  Layout(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Memory::Managed::Vector<Slot> slots,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Bool parameters)
      : domain(domain), slots(slots), anchor(anchor), parameters(parameters) {}

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& host,
      Bool parameters,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Addressable&> self) -> Bool;

  auto is_named() const -> Bool;
  auto get_slot(Count index) const -> Perimortem::Core::Option<const Slot&>;
  auto fits_value(
      const Tetrodotoxin::Source::Layout& target,
      Count source_index,
      Count target_index) const -> Bool;
  auto has_unique_names() const -> Bool;

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Memory::Managed::Vector<Slot> slots;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
  Bool parameters;
};

}  // namespace Tetrodotoxin::Library::Language::Model
