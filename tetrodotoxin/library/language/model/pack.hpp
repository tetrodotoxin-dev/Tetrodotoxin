// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/pack.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Pack is the identity-free Library lifecycle shared by every produced value
// flow. A scalar Expression and an authored parenthesized group expose the same
// link, fitting, and finalization surface while their Layout entries retain the
// real semantic identities.
//
// The TTX Pack remains the host neutral support contract. This derived owner
// adds only the Library stages needed to bind authored Expressions. Layout
// observation is total even while a Pack is incomplete. Only resolving itself
// admits that Layout as produced flow, where empty output means zero values
// rather than an incomplete sentinel.
class Pack : public Tetrodotoxin::Source::Pack {
 public:
  static auto from(Tetrodotoxin::Source::Abstract& identity)
      -> Perimortem::Core::Option<Pack&>;
  static auto from(const Tetrodotoxin::Source::Abstract& identity)
      -> Perimortem::Core::Option<const Pack&>;

  virtual auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool = 0;

  virtual auto link_restored(
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool;

  // The scalar output query is a convenience over the Pack's completed Layout.
  // It is Unknown for empty or multiple value flow and never materializes an
  // aggregate Type merely to make the query succeed.
  virtual auto get_type() const -> const Tetrodotoxin::Source::Abstract&;

  virtual auto get_result() const -> const Tetrodotoxin::Source::Abstract&;

  // Completion belongs to the concrete semantic identities referenced by the
  // Layout. It is not represented by resolving the Pack itself.
  virtual auto is_complete() const -> Bool = 0;

  virtual auto get_anchor() const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> {
    return {};
  }

  // A scalar Pack may expose the one real Abstract supplying its flow. Empty
  // and multi-value Packs have no aggregate identity.
  virtual auto get_identity() const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&>;

  template <typename Requested>
  auto select_identity() const -> Perimortem::Core::Option<const Requested&> {
    auto identity = get_identity();
    return identity ? identity->select<Requested>()
                    : Perimortem::Core::Option<const Requested&>();
  }

  template <typename Requested>
  auto select_identity() -> Perimortem::Core::Option<Requested&> {
    auto identity = get_identity();
    auto selected = identity ? identity->select<Requested>()
                             : Perimortem::Core::Option<const Requested&>();
    return selected ? Perimortem::Core::Option<Requested&>(
                          const_cast<Requested&>(*selected))
                    : Perimortem::Core::Option<Requested&>();
  }

  template <typename Requested>
  auto is_identity() const -> Bool {
    return bool(select_identity<Requested>());
  }

  // Value Type selection follows the real producer that owns each output
  // position. A raw Type identity in a Layout is not evidence that a value was
  // produced, while Call and composed Packs can expose each real result.
  virtual auto get_value_type(Count index) const
      -> const Tetrodotoxin::Source::Abstract& = 0;

  // Pack fitting preserves each real producer's value rules before falling
  // back to its identity free output Layout. This is what lets a Constant own
  // contextual scalar conversion while grouped and named flows retain their
  // concrete Layout ordering and names.
  virtual auto fits(const Tetrodotoxin::Source::Layout& target) const -> Bool;
  auto fits_at(const Tetrodotoxin::Source::Layout& target, Count target_offset) const
      -> Bool;
  auto fits_entry(
      const Tetrodotoxin::Source::Layout& target,
      Count source_index,
      Count target_index) const -> Bool;

  auto get_fitted_at(
      const Tetrodotoxin::Source::Layout& target,
      Count target_offset,
      Count target_index) const -> Perimortem::Utility::
      Result<const Tetrodotoxin::Source::Abstract&, Tetrodotoxin::Source::Layout::Errors>;

  // A general Pack fits a Type through both complete Layouts. Scalar contextual
  // conversions belong to Expression and Constant. Applying them here would
  // let single entry grouped flow ignore the rest of a structural Type Layout.
  virtual auto fits(const Tetrodotoxin::Source::Type& target) const -> Bool;

  // Receiving a Pack is target Type policy. Ordinary source fitting runs
  // first, then the selected Library Type may admit another complete flow
  // shape without changing the source Pack or its Layout.
  auto fits_into(const Tetrodotoxin::Source::Type& target) const -> Bool;

  // Finalization visits the real child Packs in evaluation order. It does not
  // imply that empty or multiple value flow can be folded into one value.
  virtual auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void = 0;

  // A composed Pack exposes its retained producers in evaluation order. Scalar
  // owners keep this view empty because their concrete semantic edges already
  // describe evaluation. Terminal producers use this query only after proving
  // that the Pack is not an Expression.
  virtual constexpr auto get_entries() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Source::PackReference<Pack>> {
    return {};
  }

  static auto create_empty(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor = {}) -> Pack&;

  // A concrete producer may compose already retained child Packs when its
  // semantic result is genuinely grouped flow. Positional composition flattens
  // child Layouts. Named composition requires one produced value per name. The
  // children remain the only value identities and evaluation edges.
  static auto create_group(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::PackReference<Pack>> entries,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names = {},
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor = {}) -> Pack&;

  // Constant evaluation composes already completed scalar Constant Packs.
  // The resulting positional Pack owns no authored linking work and is
  // immediately observable through the ordinary Pack contract.
  static auto create_folded(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::PackReference<Pack>> entries)
      -> Pack&;

  // Generated execution owners may compose an already linked named Pack.
  // Unlike authored groups, every supplied entry is already a completed graph
  // edge and no lexical pass may replace it.
  static auto create_completed(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::PackReference<Pack>> entries,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names = {})
      -> Pack&;

 protected:
  constexpr Pack() = default;
};

}  // namespace Tetrodotoxin::Library::Language::Model
