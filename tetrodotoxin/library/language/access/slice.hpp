// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Slice is the safe indexed element or contiguous range access. Element access
// supplies one scalar value. Range access supplies a fixed size Pack whose real
// producer remains this Slice expression. It does not eagerly materialize a
// View or anonymous aggregate Type. Writable reference selection belongs to the
// separate bracket access form.
class Slice : public Expression {
 public:
  TTX_CONTRACT(Slice, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Model::Pack& index,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Slice&;
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Model::Pack& start,
      Model::Pack& count,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Slice&;
  TTX_NAME("Slice"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;

  auto get_type() const -> const Tetrodotoxin::Source::Abstract& override;
  auto get_value_type(Count index) const
      -> const Tetrodotoxin::Source::Abstract& override;
  auto get_layout() const -> const Tetrodotoxin::Source::Layout& override;
  auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;
  auto fits(const Tetrodotoxin::Source::Type& target) const -> Bool override;
  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void override;

  constexpr auto get_receiver() const -> const Model::Pack& { return receiver; }

  // The first operand is the scalar index or the first position of a range.
  constexpr auto get_index() const -> const Model::Pack& { return first; }

  constexpr auto get_count() const
      -> Perimortem::Core::Option<const Model::Pack&> {
    return count.visit(
        []() -> Perimortem::Core::Option<const Model::Pack&> { return {}; },
        [](const Tetrodotoxin::Source::PackReference<Model::Pack>& selected)
            -> Perimortem::Core::Option<const Model::Pack&> {
          return selected.get();
        });
  }

  constexpr auto get_element_type() const
      -> Perimortem::Core::Option<const Model::Type&> {
    return element_type.visit(
        []() -> Perimortem::Core::Option<const Model::Type&> { return {}; },
        [](const Tetrodotoxin::Source::Reference<const Model::Type>& selected)
            -> Perimortem::Core::Option<const Model::Type&> {
          return selected.get();
        });
  }

  constexpr auto get_fallback() const
      -> Perimortem::Core::Option<const Model::Pack&> {
    return fallback.visit(
        []() -> Perimortem::Core::Option<const Model::Pack&> { return {}; },
        [](const Tetrodotoxin::Source::PackReference<Model::Pack>& selected)
            -> Perimortem::Core::Option<const Model::Pack&> {
          return selected.get();
        });
  }

  constexpr auto get_range_count() const -> Perimortem::Core::Option<Count> {
    return range_count;
  }

 protected:
  auto evaluate() -> Perimortem::Utility::Result<
      Perimortem::Core::Option<Model::Pack&>,
      Expression::Error> override;

 private:
  Slice(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Model::Pack& index,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor);
  Slice(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Model::Pack& start,
      Model::Pack& count,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor);

  Perimortem::Memory::Allocator::Arena& domain;
  Model::Pack& receiver;
  Model::Pack& first;
  Perimortem::Core::Option<Tetrodotoxin::Source::PackReference<Model::Pack>> count;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Model::Type>>
      element_type;
  Perimortem::Core::Option<Tetrodotoxin::Source::PackReference<Model::Pack>> fallback;
  Perimortem::Core::Option<Count> range_count;
  Perimortem::Core::Option<const Tetrodotoxin::Source::Layout&> range_layout;
};

}  // namespace Tetrodotoxin::Library::Language::Access
