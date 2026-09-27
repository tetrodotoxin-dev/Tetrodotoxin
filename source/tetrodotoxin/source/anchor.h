// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_SOURCE_ANCHOR_H
#define TETRODOTOXIN_SOURCE_ANCHOR_H

#include "tetrodotoxin/source/range.h"
#include "ttx/concept/abstract.h"

#ifdef __cplusplus
#include "perimortem/core/option.hpp"
#include "ttx/concept/abstract.hpp"
#endif

// A byte offset alone cannot tell an editor which revision it describes.
// Anchor keeps the exact Source observation beside its extent, allowing old
// declarations to remain meaningful after a host publishes replacement text.
// The enclosing publication retains that observation and its executable code
// while any borrowed Anchor is in use.
//
// Extent describes the relevant source region. Optional focus selects the part
// a diagnostic should emphasize and may be a zero length caret. Absence of
// focus is distinct from that caret. Focus need not lie inside extent because
// the presenting policy decides which evidence is useful to show together.
// Neither range depends on token classification or cached line numbers.
typedef struct tetrodotoxin_source_anchor {
  ttx_abstract source;
  tetrodotoxin_source_range extent;
  tetrodotoxin_source_range focus;
  U8 has_focus;

#ifdef __cplusplus
  constexpr tetrodotoxin_source_anchor(
      Ttx::Concept::Abstract source,
      tetrodotoxin_source_range extent,
      Perimortem::Core::Option<tetrodotoxin_source_range> focus = {})
      : source(source.get_abi()), extent(extent),
        focus(focus ? *focus : tetrodotoxin_source_range()),
        has_focus(focus ? 1 : 0) {}

  constexpr auto get_source() const -> Ttx::Concept::Abstract {
    return Ttx::Concept::Abstract(source);
  }
  constexpr auto get_extent() const -> tetrodotoxin_source_range {
    return extent;
  }
  constexpr auto get_focus() const
      -> Perimortem::Core::Option<tetrodotoxin_source_range> {
    if (has_focus) {
      return focus;
    }

    return {};
  }
#endif
} tetrodotoxin_source_anchor;

#endif
