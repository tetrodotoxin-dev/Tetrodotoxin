// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialects/source/anchor.h"
#include "tetrodotoxin/dialects/source/snapshot.h"
#include "ttx/concept/abstract.h"

#define TETRODOTOXIN_SOURCE_DIAGNOSTICS_ID_HIGH 0xdf16a8d9c76940c7ULL
#define TETRODOTOXIN_SOURCE_DIAGNOSTICS_ID_LOW 0xa582a564c7d9b982ULL

// Diagnostics consumes source evidence and message bytes synchronously. The
// caller lends every view until report returns. A sink retaining any evidence
// copies it or acquires its own lifetime. count observes completed reports so
// an enclosing parser can avoid repeating a more precise nested failure.
// A path is a display label. A sink combining successive source versions must
// keep their snapshots distinct. It must not retain the caller's Parse state.
typedef struct tetrodotoxin_source_diagnostics {
  const void* source;
  const struct tetrodotoxin_source_diagnostics_ops* operations;
} tetrodotoxin_source_diagnostics;
typedef struct tetrodotoxin_source_diagnostics_ops {
  ttx_abstract_ops abstract;
  void (*report)(
      const void* source,
      tetrodotoxin_source_snapshot input,
      tetrodotoxin_source_anchor anchor,
      perimortem_view_bytes message,
      perimortem_view_bytes hint);
  Count (*count)(const void* source);
} tetrodotoxin_source_diagnostics_ops;

PERIMORTEM_C const ttx_representation*
    tetrodotoxin_source_diagnostics_representation(void);
