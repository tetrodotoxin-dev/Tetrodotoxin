// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialects/source/cursor.h"
#include "ttx/concept/abstract.h"

#define TETRODOTOXIN_SOURCE_PARSE_ID_HIGH 0xe447c30e66854e9fULL
#define TETRODOTOXIN_SOURCE_PARSE_ID_LOW 0x8018c38be2d0a453ULL

// Parse interprets a caller owned token range. The provider negotiates the
// Stream's lexical identity and representation, and the supplied Diagnostics
// subject, before consumption. Unsupported input or a refusing diagnostics
// policy returns without moving the cursor or calling receive.
//
// Once admitted, parsing can advance the cursor and publish diagnostics even
// when no projection can be produced. Those effects are not rolled back.
// Satisfied calls receive exactly once with a plain borrowed Abstract. It may
// represent partial meaning and promises no executable or complete program.
// Unknown and Rejected never call receive. A receiver retaining the result
// acquires Borrow during that callback and keeps provider code loaded through
// release. Neither cursor, receiver nor diagnostics is implicitly retained.
// Nested parsers share index. The provider preserves stream, begin and end.
typedef struct tetrodotoxin_source_parse {
  const void* source;
  const struct tetrodotoxin_source_parse_ops* operations;
} tetrodotoxin_source_parse;
typedef struct tetrodotoxin_source_parse_ops {
  ttx_abstract_ops abstract;
  ttx_binding_status (*parse)(
      const void* source,
      tetrodotoxin_source_cursor* cursor,
      ttx_abstract diagnostics,
      void* receiver,
      void (*receive)(void* receiver, ttx_abstract result));
} tetrodotoxin_source_parse_ops;

PERIMORTEM_C const ttx_representation* tetrodotoxin_source_parse_representation(
    void);
