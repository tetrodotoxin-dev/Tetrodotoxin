// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/language/attribute.hpp"
#include "tetrodotoxin/language/type_reference.hpp"
#include "tetrodotoxin/render/language/binding.hpp"
#include "tetrodotoxin/source/layout.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Render::Language {

// Layout is the data flow projection of one Render Stage interface. Each slot
// retains its authored Type route and links to one real parameter Binding or
// result Type. The TTX Layout surface exposes only names, order, and fitting.
//
// Render Attributes remain beside that projection because locations, builtins,
// and resource policy carry meaning that Layout intentionally drops. Interface
// negotiation combines both views without asking Shader to reuse this concrete
// descriptor as its own executable signature.
class Layout : public Tetrodotoxin::Source::Layout {
 public:
  class Slot {
   public:
    constexpr Slot(
        Tetrodotoxin::Language::TypeReference type,
        Perimortem::Core::View::Bytes name,
        Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
            attributes,
        Tetrodotoxin::Source::Lexical::Anchor anchor)
        : type(type), name(name), attributes(attributes), anchor(anchor) {}

    constexpr auto get_type() const
        -> const Tetrodotoxin::Language::TypeReference& {
      return type;
    }

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }

    constexpr auto get_attributes() const
        -> Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute> {
      return attributes;
    }

    constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

    constexpr auto get_edge() const
        -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> {
      return edge.visit(
          []() -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> {
            return {};
          },
          [](const Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>&
                 selected)
              -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> {
            return selected.get();
          });
    }

    auto retain_edge(const Tetrodotoxin::Source::Abstract& selected) -> Bool {
      if (edge) {
        return False;
      }
      edge = Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>(selected);
      return True;
    }

   private:
    Tetrodotoxin::Language::TypeReference type;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
        attributes;
    Tetrodotoxin::Source::Lexical::Anchor anchor;
    Perimortem::Core::Option<
        Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>>
        edge;
  };

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Memory::Managed::Vector<Slot> slots,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Bool parameters) -> Layout&;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor, const Tetrodotoxin::Source::Abstract& context)
      -> Bool;

  auto link_restored(const Tetrodotoxin::Source::Abstract& context) -> Bool;

  auto resolve_named(Perimortem::Core::View::Bytes name) const
      -> const Tetrodotoxin::Source::Abstract&;

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

  constexpr auto contains_parameters() const -> Bool { return parameters; }

  constexpr auto get_slots() const -> Perimortem::Core::View::Vector<Slot> {
    return slots;
  }

  auto is_linked() const -> Bool;

  auto get_size() const -> Count override;

  auto get_abstract(Count index) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> override;

  auto get_name(Count index) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> override;

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

  auto find_target(
      const Tetrodotoxin::Source::Layout& target,
      Count source_index,
      Count target_offset) const -> Perimortem::Core::Option<Count>;

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Memory::Managed::Vector<Slot> slots;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
  Bool parameters;
};

}  // namespace Tetrodotoxin::Render::Language
