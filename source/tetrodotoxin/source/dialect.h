// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_SOURCE_DIALECT_H
#define TETRODOTOXIN_SOURCE_DIALECT_H

#include "tetrodotoxin/source/lexical/cursor.h"
#include "ttx/semantic/ownership/publication.h"

#define TETRODOTOXIN_SOURCE_DIALECT_ID_HIGH 0x63dfa3a2980e43baULL
#define TETRODOTOXIN_SOURCE_DIALECT_ID_LOW 0xb299e1bd19e49b42ULL

// A dialect executes a range of a command stream. It may build a model or
// perform other synchronous effects, with its returned Publication supplying
// the promises that consumers can subsequently inspect. The Cursor is already
// acquired. Interpretation advances the index in the supplied Cursor record,
// leaving its receiver and operations unchanged. The caller passes its own
// record to continue traversal or a copy to explore a fork. Both share the
// provider's diagnostic context.
//
// The host selects the lexical policy whose Code vocabulary the dialect uses.
// Nested dialects receive whichever positioned Cursor their caller lends.
// The context supplies the scope and capabilities selected by the host, rather
// than prescribing a native environment or allocator.
//
// The retained result of one invocation is its Monograph: an Abstract policy
// exposed through the returned Publication's Query. It can own native storage,
// retain child publications, or forward another dialect's result unchanged.
// An enclosing Monograph chooses how those children enter its visible surface.
// This invocation boundary does not identify a file or require a source root.
// Publication supplies the release obligation; Abstract supplies negotiation
// and navigation, so composition needs no native Monograph base or extra table.
//
// Satisfied transfers one Publication. Other outcomes leave output untouched.
// They retain the consumed index and do not undo effects already performed.
// Diagnostics remain in its error context, including when interpretation rejects
// input.
//
// Output owners copy any spelling they retain beyond interpretation. Their
// Source Anchors and semantic references still borrow the original observations
// and context, which the caller retains until the output is released. Provider
// code must remain loaded through that release. Cursor itself is borrowed only
// for this call and can be released before the returned graph is inspected.
typedef struct tetrodotoxin_source_dialect {
  const void* source;
  ttx_binding_status (*interpret)(const void* source,
      tetrodotoxin_source_cursor* cursor, ttx_abstract context,
      ttx_publication* output);
} tetrodotoxin_source_dialect;

PERIMORTEM_C const ttx_representation* tetrodotoxin_source_dialect_representation(void);

#endif
