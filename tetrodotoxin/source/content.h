// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_SOURCE_CONTENT_H
#define TETRODOTOXIN_SOURCE_CONTENT_H

#include "ttx/semantic/negotiation/query.h"

#define TETRODOTOXIN_SOURCE_CONTENT_ID_HIGH 0x86ac8d06ca0f4a4eULL
#define TETRODOTOXIN_SOURCE_CONTENT_ID_LOW 0x812ea48cb92ee378ULL

// An editor buffer, file or generated source can supply the same immutable
// byte observation without choosing the language that will interpret it.
// Content gives that observation a size and a data Query. Consumers request
// a contiguous U8 range of exactly that size through the existing Data
// protocols, allowing a provider to copy bytes without exposing its storage.
// Empty content is a valid observation and uses the empty Data form.
//
// The Query and bytes borrow the Source publication. Their size and content
// remain unchanged for its lifetime. An edit publishes another observation,
// leaving retained Anchors attached to their original bytes. Paths, encodings
// and tokenization policies are separate questions from this byte contract.
typedef struct tetrodotoxin_source_content {
  const void* source;
  U64 (*get_size)(const void* source);
  ttx_semantic_query (*get_data)(const void* source);
} tetrodotoxin_source_content;

PERIMORTEM_C const ttx_representation*
tetrodotoxin_source_content_representation(void);

#endif
