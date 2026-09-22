// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/source/constant.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Aggregate is one immutable fact for complete empty or multi-value folded
// flow. Its Layout keeps the exact child Constants and optional names; the
// aggregate adds no proxy value identities.
class Aggregate final : public Tetrodotoxin::Source::Constant, public Model::Pack {
 public:
  TTX_CONTRACT(Aggregate, Tetrodotoxin::Source::Constant);

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>>
          values,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names = {})
      -> Perimortem::Core::Option<Aggregate&>;

  auto get_name() const -> Perimortem::Core::View::Bytes override;
  TTX_EMPTY_DOCUMENTATION();

  auto get_type() const -> const Tetrodotoxin::Source::Abstract& override;
  auto get_result() const -> const Tetrodotoxin::Source::Abstract& override;
  auto get_identity() const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> override;
  auto get_layout() const -> const Tetrodotoxin::Source::Layout& override;
  auto get_value_type(Count index) const
      -> const Tetrodotoxin::Source::Abstract& override;
  constexpr auto is_complete() const -> Bool override { return True; }
  auto fits(const Tetrodotoxin::Source::Layout& target) const -> Bool override;
  auto fits(const Tetrodotoxin::Source::Type& target) const -> Bool override;
  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;
  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void override;

 private:
  class Layout final : public Tetrodotoxin::Source::Layout {
   public:
    constexpr explicit Layout(const Aggregate& aggregate)
        : aggregate(aggregate) {}

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
        Count target_index) const -> Perimortem::Utility::
        Result<const Tetrodotoxin::Source::Abstract&, Errors> override;

   private:
    const Aggregate& aggregate;
  };

  Aggregate(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>>
          values,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names);

  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>>
      values;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes> names;
  Perimortem::Memory::Managed::Bytes name;
  Layout layout;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
