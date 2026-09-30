// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialects/source/snapshot.h"
#include "tetrodotoxin/dialects/source/tokens.h"
#include "ttx/concept/abstract.h"

#define TETRODOTOXIN_SOURCE_STREAM_ID_HIGH 0xe685c19665bf48c9ULL
#define TETRODOTOXIN_SOURCE_STREAM_ID_LOW 0xa89efa9e125a5793ULL

// Stream observes immutable source bytes and classified tokens together.
// Tokens address input.text by byte offset. Its lexical identity establishes
// code meanings independently of the described physical token representation.
// Binding copies a view only. Retention requires Borrow from the same policy.
// All returned views survive for the supplying observation or acquisition.
typedef struct tetrodotoxin_source_stream {
  const void* source;
  const struct tetrodotoxin_source_stream_ops* operations;
} tetrodotoxin_source_stream;
typedef struct tetrodotoxin_source_stream_ops {
  ttx_abstract_ops abstract;
  tetrodotoxin_source_snapshot (*input)(const void* source);
  tetrodotoxin_source_tokens (*tokens)(const void* source);
  perimortem_uuid (*lexicon)(const void* source);
} tetrodotoxin_source_stream_ops;

PERIMORTEM_C const ttx_representation*
    tetrodotoxin_source_stream_representation(void);
