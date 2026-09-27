// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/layouts/addressable.hpp"
#include "tetrodotoxin/source/layouts/named.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// RangeLoop owns one authored `for` statement and its complete binding Layout.
// Each binding is one real Layout-owned Addressable retained by the loop, while
// the selected input Type owns which Layouts it can produce during iteration.
class RangeLoop : public Tetrodotoxin::Source::Abstract {
 public:
  // AuthoredBinding is the retained source description for one loop entry.
  // Linking replaces each delayed Type route with a real Addressable while
  // this evidence remains available for diagnostics and reflection.
  struct AuthoredBinding {
    Tetrodotoxin::Source::Lexical::Token name_token;
    Perimortem::Core::View::Bytes name;
    TypeReference type_reference;
  };

  TTX_CONTRACT(RangeLoop, Tetrodotoxin::Source::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Block& lexical_context,
      Perimortem::Core::View::Vector<AuthoredBinding> bindings,
      Model::Pack& input,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> RangeLoop&;

  auto complete_body(Block& selected, Tetrodotoxin::Source::Lexical::Anchor selected_anchor)
      -> Bool;

  RangeLoop(const RangeLoop&) = delete;
  RangeLoop(RangeLoop&&) = delete;
  auto operator=(const RangeLoop&) -> RangeLoop& = delete;
  auto operator=(RangeLoop&&) -> RangeLoop& = delete;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor, const Model::Type& access_scope)
      -> Bool;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void;

  TTX_NAME("For"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  auto resolve_authored_context(
      Perimortem::Core::View::Bytes route,
      Count offset) const -> const Tetrodotoxin::Source::Abstract&;

  constexpr auto get_input() const -> const Model::Pack& { return input.get(); }

  constexpr auto get_input_type() const
      -> Perimortem::Core::Option<const Model::Type&> {
    return input_type.visit(
        []() -> Perimortem::Core::Option<const Model::Type&> { return {}; },
        [](const Tetrodotoxin::Source::Reference<const Model::Type>& selected)
            -> Perimortem::Core::Option<const Model::Type&> {
          return selected.get();
        });
  }

  constexpr auto get_bindings() const -> const Tetrodotoxin::Source::Layout& {
    return *binding_layout;
  }

  constexpr auto get_body() const -> const Block& { return body->get(); }

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

 private:
  RangeLoop(
      Perimortem::Memory::Allocator::Arena& domain,
      Block& lexical_context,
      Perimortem::Core::View::Vector<AuthoredBinding> bindings,
      Model::Pack& input,
      Tetrodotoxin::Source::Lexical::Anchor anchor);

  Perimortem::Memory::Allocator::Arena& domain;
  Block& lexical_context;
  Perimortem::Memory::Managed::Vector<AuthoredBinding> authored_bindings;
  Perimortem::Memory::Managed::Vector<
      Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Layouts::Addressable>>
      bindings;
  Perimortem::Memory::Managed::Vector<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>>
      binding_entries;
  Perimortem::Core::Option<Tetrodotoxin::Source::Layouts::Named> binding_layout;
  Tetrodotoxin::Source::PackReference<Model::Pack> input;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<Block>> body;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Model::Type>>
      input_type;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
