// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_MODEL_CURSOR_H
#define VALIDATION_MODEL_CURSOR_H

#include "tetrodotoxin/source/dialect.h"

// This fixture supplies an already classified command stream, as a foreign
// preprocessor could. Its C implementation owns spelling, ranges and diagnostics;
// consumers carry their positions. It deliberately uses descending, nonzero
// Token locators, distinct from those positions.
typedef struct cursor_fixture_entry {
  U8 code;
  tetrodotoxin_source_range extent;
} cursor_fixture_entry;

PERIMORTEM_C ttx_publication cursor_fixture_open(
    ttx_abstract origin, perimortem_view_bytes text,
    const cursor_fixture_entry* entries, Count count, U32* releases);
PERIMORTEM_C ttx_binding_status cursor_fixture_interpret(
    ttx_semantic_query dialect, tetrodotoxin_source_cursor* cursor,
    ttx_abstract context, ttx_publication* output);

// Count indexed observations inside the loaded C provider. This distinguishes
// host caching from a provider silently caching its own callback work.
PERIMORTEM_C U64 cursor_fixture_observations(ttx_semantic_query query);

#endif
