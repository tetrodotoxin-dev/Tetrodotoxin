// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/layout.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// Return is one concrete terminal statement. It retains one real Pack while the
// enclosing Block supplies lexical lookup, host access, and the Function result
// Layout required during linking. A bare `return` owns an empty Pack, so empty
// flow and flow with several values share one lifecycle without optional state.
class Return : public Tetrodotoxin::Source::Abstract {
 public:
  TTX_CONTRACT(Return, Tetrodotoxin::Source::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Model::Pack& pack) -> Return&;

  Return(const Return&) = delete;
  Return(Return&&) = delete;
  auto operator=(const Return&) -> Return& = delete;
  auto operator=(Return&&) -> Return& = delete;

  auto link(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Abstract& lexical_context,
      const Model::Type& access_scope,
      const Tetrodotoxin::Source::Layout& results) -> Bool;

  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void;

  TTX_NAME("Return"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }
  constexpr auto get_pack() const -> const Model::Pack& { return pack.get(); }

 private:
  constexpr Return(
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Tetrodotoxin::Source::PackReference<Model::Pack> pack)
      : anchor(anchor), pack(pack) {}

  Tetrodotoxin::Source::Lexical::Anchor anchor;
  Tetrodotoxin::Source::PackReference<Model::Pack> pack;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
