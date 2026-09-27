// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_SOURCE_DIAGNOSTIC_H
#define TETRODOTOXIN_SOURCE_DIAGNOSTIC_H

#include "perimortem/core/view/bytes.h"
#include "tetrodotoxin/source/anchor.h"

#ifdef __cplusplus
#include "perimortem/core/view/bytes.hpp"
#endif

// Reporting copies message bytes into the Errors owner. Reading lends them
// back, so a consumer can inspect diagnostics without knowing that owner's
// allocator. The optional Anchor borrows its Source observation. Retaining a
// diagnostic beyond the error context requires copying its bytes and retaining
// that observation separately.
typedef struct tetrodotoxin_source_diagnostic {
  tetrodotoxin_source_anchor anchor;
  perimortem_view_bytes message;
  perimortem_view_bytes hint;
  U8 has_anchor;

#ifdef __cplusplus
  tetrodotoxin_source_diagnostic()
      : anchor(Ttx::Concept::Abstract(ttx_none()), tetrodotoxin_source_range()),
        message{}, hint{}, has_anchor(0) {}
  tetrodotoxin_source_diagnostic(
      Perimortem::Core::Option<tetrodotoxin_source_anchor> location,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint)
      : anchor(location ? *location : tetrodotoxin_source_anchor(Ttx::Concept::Abstract(ttx_none()), tetrodotoxin_source_range())),
        message{message.get_data(), message.get_size()},
        hint{hint.get_data(), hint.get_size()}, has_anchor(location ? 1 : 0) {}
  auto get_anchor() const -> Perimortem::Core::Option<tetrodotoxin_source_anchor> {
    if (has_anchor) { return anchor; }
    return {};
  }
  auto get_message() const -> Perimortem::Core::View::Bytes { return {message.data, message.size}; }
  auto get_hint() const -> Perimortem::Core::View::Bytes { return {hint.data, hint.size}; }
#endif
} tetrodotoxin_source_diagnostic;

#endif
