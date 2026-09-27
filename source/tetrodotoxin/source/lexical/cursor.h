// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_SOURCE_LEXICAL_CURSOR_H
#define TETRODOTOXIN_SOURCE_LEXICAL_CURSOR_H

#include "tetrodotoxin/source/diagnostic.h"
#include "tetrodotoxin/source/lexical/span.h"

#define TETRODOTOXIN_SOURCE_LEXICAL_CURSOR_ID_HIGH 0x93e1bacfb5f64673ULL
#define TETRODOTOXIN_SOURCE_LEXICAL_CURSOR_ID_LOW 0x89b4267a4ed0d9f2ULL

// A dialect needs to explore input without advancing every other consumer of
// the same source. Cursor therefore carries its own logical token index, while
// the provider supplies an immutable token sequence and diagnostic context.
// Copying this record forks traversal without acquiring a new interface. A
// consuming call receives a pointer to the record whose index it may advance;
// its caller can supply either the original Cursor or a local fork.
//
// Index counts logical positions from the start of the sequence. It is not a
// Token locator: only the provider interprets locators. The operation table and
// receiver borrow the provider's publication and executable code. Binding
// describes the pointed-to table's full callable signatures before use.
typedef struct tetrodotoxin_source_cursor {
  const void* source;
  const struct tetrodotoxin_source_cursor_ops* operations;
  U64 index;
} tetrodotoxin_source_cursor;

// get_token observes an absolute index without advancing anything. Repeated
// observations at that index return the same Token for the publication's
// lifetime, so a host can cache its current value. Indices at or beyond the
// sequence's end return its Terminal Token. Reading before the caller's
// admitted input range remains outside the contract.
//
// Tokens and spans are valid with any Cursor over their supplying sequence.
// get_text lends bytes until provider publication release, including across
// later observations and advances.
// Providers must retain generated spelling for that lifetime. A consumer that
// keeps text in an emitted graph copies it into its own storage.
// Anchors borrow the original Source observation rather than traversal state.
// Its publication must outlive any graph that retains those Anchors.
//
// report copies message and hint bytes before returning. optional_anchor may
// be null for a general error. The Errors context owns the reports. get_error
// lends their bytes and writes output only when the requested report exists.
// Forking position still shares diagnostics; it does not roll back reports or
// other dialect effects when the fork is discarded.
// Cursor acquisition retains neither a token array view nor a native allocator.
// Token is an eight-byte value returned directly. Larger provenance and
// diagnostic records use caller-owned output.
typedef struct tetrodotoxin_source_cursor_ops {
  tetrodotoxin_source_token (*get_token)(const void* source, U64 index);
  perimortem_view_bytes (*get_text)(const void* source, tetrodotoxin_source_token token);
  void (*get_anchor)(const void* source, tetrodotoxin_source_span span,
                     const tetrodotoxin_source_token* optional_focus,
                     tetrodotoxin_source_anchor* output);
  U64 (*get_error_count)(const void* source);
  void (*report)(const void* source, const tetrodotoxin_source_anchor* optional_anchor,
                 perimortem_view_bytes message, perimortem_view_bytes hint);
  U8 (*get_error)(const void* source, U64 index, tetrodotoxin_source_diagnostic* output);
} tetrodotoxin_source_cursor_ops;

PERIMORTEM_C const ttx_representation* tetrodotoxin_source_cursor_representation(void);

#endif
