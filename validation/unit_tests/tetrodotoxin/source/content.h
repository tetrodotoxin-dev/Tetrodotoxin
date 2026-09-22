// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_SOURCE_CONTENT_H
#define VALIDATION_SOURCE_CONTENT_H

#include "tetrodotoxin/source/content.h"
#include "tetrodotoxin/source/tokenization.h"
#include "ttx/concept/abstract.h"
#include "ttx/semantic/ownership/publication.h"

// A C observation generates a repeated alphabet when asked to copy its bytes.
// There is no backing source buffer for the consumer to recover with a cast.
PERIMORTEM_C ttx_publication source_content_open(
    const ttx_representation* form, Count size, U8 first,
    ttx_binding_status acceptance, U8 fail_read, U32* releases);

PERIMORTEM_C ttx_semantic_query source_tokenization_query(
    const tetrodotoxin_source_tokenization* service);

PERIMORTEM_C U32 source_content_reads(ttx_semantic_query query);

#endif
