// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_SOURCE_LEXICAL_PUBLICATION_H
#define VALIDATION_SOURCE_LEXICAL_PUBLICATION_H

#include "tetrodotoxin/source/lexical/cursor.h"
#include "tetrodotoxin/source/dialect.h"
#include "ttx/semantic/negotiation/query.h"

// A C consumer requests the same complete Cursor record as the C++ facade.
PERIMORTEM_C ttx_binding_status source_read_cursor(
    ttx_semantic_query query, tetrodotoxin_source_cursor* output);

// C enriches the same indexed provider with consumption. Advancing this record
// changes only its position, and the returned bits let C++ check the observation.
PERIMORTEM_C U64 source_consume_cursor(
    tetrodotoxin_source_cursor* cursor);

// A foreign dialect can consume valid input before declining to publish.
// Its returned index must survive failure and invalidate any host-side cache.
PERIMORTEM_C tetrodotoxin_source_dialect source_rejecting_dialect(void);

#endif
