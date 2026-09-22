// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/callable.hpp"

namespace Tetrodotoxin::App::Language {

// Program retains one authored Package route and the exact Static Callable
// selected from that completed Package graph during linking.
class Program : public Tetrodotoxin::Source::Abstract {
 public:
  TTX_CONTRACT(Program, Tetrodotoxin::Source::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Documentation& documentation,
      Perimortem::Core::View::Bytes route,
      Perimortem::Core::View::Bytes callable_name,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Tetrodotoxin::Source::Lexical::Anchor selection_anchor) -> Program&;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor, Tetrodotoxin::Source::Abstract& context)
      -> Bool;

  TTX_NAME("Program"_view);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }

  constexpr auto get_callable_name() const -> Perimortem::Core::View::Bytes {
    return callable_name;
  }

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

  constexpr auto get_entry() const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Callable&> {
    return entry ? Perimortem::Core::Option<const Tetrodotoxin::Source::Callable&>(
                       entry->get())
                 : Perimortem::Core::Option<const Tetrodotoxin::Source::Callable&>();
  }

  auto resolve_concept(Perimortem::Core::View::Bytes) const
      -> const Tetrodotoxin::Source::Abstract& override;

 private:
  constexpr Program(
      const Tetrodotoxin::Source::Documentation& documentation,
      Perimortem::Core::View::Bytes route,
      Perimortem::Core::View::Bytes callable_name,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Tetrodotoxin::Source::Lexical::Anchor selection_anchor)
      : documentation(documentation),
        route(route),
        callable_name(callable_name),
        anchor(anchor),
        selection_anchor(selection_anchor) {}

  const Tetrodotoxin::Source::Documentation& documentation;
  Perimortem::Core::View::Bytes route;
  Perimortem::Core::View::Bytes callable_name;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
  Tetrodotoxin::Source::Lexical::Anchor selection_anchor;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Callable>>
      entry;
};

}  // namespace Tetrodotoxin::App::Language
