// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"

namespace Tetrodotoxin::App::Language {

// Runtime retains target neutral application startup policy. Profile specific
// settings stay with their one owner and carry no host handle or backend state.
class Runtime : public Tetrodotoxin::Source::Abstract {
 public:
  enum class Profile : U8 {
    Terminal = 1,
    Headless = 2,
    Windowed = 3,
  };

  // Windowed keeps only authored policy. Optional settings preserve the grammar
  // without inventing defaults that belong to a later runtime contract.
  class Windowed {
   public:
    constexpr Windowed(
        Perimortem::Core::Option<Perimortem::Core::View::Bytes> title,
        Perimortem::Core::Option<Perimortem::Core::View::Bytes> icon_route,
        Perimortem::Core::Option<
            Tetrodotoxin::Source::Reference<const Tetrodotoxin::Language::Resource>>
            icon,
        Perimortem::Core::Option<U32> width,
        Perimortem::Core::Option<U32> height,
        Perimortem::Core::Option<Bool> resizable)
        : title(title),
          icon_route(icon_route),
          icon(icon),
          width(width),
          height(height),
          resizable(resizable) {}

    constexpr auto get_title() const
        -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
      return title;
    }

    constexpr auto get_icon_route() const
        -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
      return icon_route;
    }

    constexpr auto get_icon() const
        -> Perimortem::Core::Option<const Tetrodotoxin::Language::Resource&> {
      return icon.visit(
          []() -> Perimortem::Core::Option<
                   const Tetrodotoxin::Language::Resource&> { return {}; },
          [](const Tetrodotoxin::Source::Reference<
              const Tetrodotoxin::Language::Resource>& selected)
              -> Perimortem::Core::Option<
                  const Tetrodotoxin::Language::Resource&> {
            return selected.get();
          });
    }

    constexpr auto get_width() const -> Perimortem::Core::Option<U32> {
      return width;
    }

    constexpr auto get_height() const -> Perimortem::Core::Option<U32> {
      return height;
    }

    constexpr auto get_resizable() const -> Perimortem::Core::Option<Bool> {
      return resizable;
    }

   private:
    Perimortem::Core::Option<Perimortem::Core::View::Bytes> title;
    Perimortem::Core::Option<Perimortem::Core::View::Bytes> icon_route;
    Perimortem::Core::Option<
        Tetrodotoxin::Source::Reference<const Tetrodotoxin::Language::Resource>>
        icon;
    Perimortem::Core::Option<U32> width;
    Perimortem::Core::Option<U32> height;
    Perimortem::Core::Option<Bool> resizable;
  };

  TTX_CONTRACT(Runtime, Tetrodotoxin::Source::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Documentation& documentation,
      Profile profile,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Runtime&;

  static auto create_windowed(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Perimortem::Core::Option<Perimortem::Core::View::Bytes> title,
      Perimortem::Core::Option<Perimortem::Core::View::Bytes> icon_route,
      Perimortem::Core::Option<const Tetrodotoxin::Language::Resource&> icon,
      Perimortem::Core::Option<U32> width,
      Perimortem::Core::Option<U32> height,
      Perimortem::Core::Option<Bool> resizable) -> Runtime&;

  // Format 1 App payloads contain only the accepted Terminal profile.
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Source::Documentation& documentation) -> Runtime&;

  auto get_name() const -> Perimortem::Core::View::Bytes override;
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

  constexpr auto get_profile() const -> Profile { return profile; }

  constexpr auto get_windowed() const
      -> Perimortem::Core::Option<const Windowed&> {
    return windowed;
  }

  auto resolve_concept(Perimortem::Core::View::Bytes) const
      -> const Tetrodotoxin::Source::Abstract& override;

 private:
  constexpr Runtime(
      const Tetrodotoxin::Source::Documentation& documentation,
      Profile profile,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Perimortem::Core::Option<const Windowed&> windowed)
      : documentation(documentation),
        profile(profile),
        anchor(anchor),
        windowed(windowed) {}

  const Tetrodotoxin::Source::Documentation& documentation;
  Profile profile;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
  Perimortem::Core::Option<const Windowed&> windowed;
};

}  // namespace Tetrodotoxin::App::Language
