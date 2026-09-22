// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Index is the reference only scalar or ranged selector for Access storage.
// Runtime bounds engage a scalar address or the complete requested interval.
// an invalid target writes nothing. Index never manufactures an Addressable or
// Option and never supplies an ordinary read. Safe value selection belongs to
// Slice's `:[...]` forms.
class Index : public Expression {
 public:
  TTX_CONTRACT(Index, Expression);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Model::Pack& index,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Index&;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& receiver,
      Model::Pack& start,
      Model::Pack& count,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Index&;

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME("Index"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto get_type() const -> const Tetrodotoxin::Source::Abstract& override;
  auto get_write_type(const Model::Type& access_scope) const
      -> Perimortem::Core::Option<const Model::Type&> override;
  auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;
  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void override;

  constexpr auto get_receiver() const -> const Model::Pack& { return receiver; }
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
  constexpr auto get_range_count() const -> Perimortem::Core::Option<Count> {
    return range_count;
  }
  auto get_element_type() const -> const Tetrodotoxin::Source::Abstract&;

 protected:
  auto link_write_target(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      const Model::Type& access_scope) -> Bool override;
  auto accepts_write(const Model::Pack& source, const Model::Type& access_scope)
      const -> Bool override;

 private:
  constexpr Index(
      Model::Pack& receiver,
      Model::Pack& index,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Expression(anchor), receiver(receiver), first(index) {}
  constexpr Index(
      Model::Pack& receiver,
      Model::Pack& start,
      Model::Pack& count,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Expression(anchor),
        receiver(receiver),
        first(start),
        count(Tetrodotoxin::Source::PackReference<Model::Pack>(count)) {}

  auto link_target(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> access_scope)
      -> Bool;

  Model::Pack& receiver;
  Model::Pack& first;
  Perimortem::Core::Option<Tetrodotoxin::Source::PackReference<Model::Pack>> count;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Model::Type>>
      element_type;
  Perimortem::Core::Option<Count> range_count;
};

}  // namespace Tetrodotoxin::Library::Language::Access
