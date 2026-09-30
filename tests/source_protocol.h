// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once
#include "tetrodotoxin/dialects/source/capabilities/diagnostics.h"
#include "tetrodotoxin/dialects/source/capabilities/parse.h"
#include "tetrodotoxin/dialects/source/token.h"

// This C fixture borrows caller supplied descriptors and arrays. None of its
// providers uses a C++ facade or Arena. It refuses selected policies explicitly
// so callers can prove that admission never bypasses their supplied subject.
typedef struct source_protocol {
  const ttx_representation* stream_representation;
  const ttx_representation* token_representation;
  const ttx_representation* parse_representation;
  const ttx_representation* diagnostics_representation;
  tetrodotoxin_source_snapshot input;
  tetrodotoxin_source_tokens tokens;
  perimortem_uuid lexicon;
  int refuse_diagnostics;
  int refuse_stream;
  Count reports;
  U8 message[64];
  U8 evidence[64];
} source_protocol;

PERIMORTEM_C tetrodotoxin_source_stream
    source_protocol_stream(source_protocol* state);
PERIMORTEM_C tetrodotoxin_source_parse
    source_protocol_parse(source_protocol* state);
PERIMORTEM_C tetrodotoxin_source_diagnostics
    source_protocol_diagnostics(source_protocol* state);
