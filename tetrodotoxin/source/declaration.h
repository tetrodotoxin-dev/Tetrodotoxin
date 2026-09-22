// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_SOURCE_DECLARATION_H
#define TETRODOTOXIN_SOURCE_DECLARATION_H

#include "tetrodotoxin/source/anchor.h"
#include "ttx/semantic/negotiation/query.h"

#define TETRODOTOXIN_SOURCE_DECLARATION_ID_HIGH 0x01a087abcbf97905ULL
#define TETRODOTOXIN_SOURCE_DECLARATION_ID_LOW 0x9ea29543510280bbULL

// A declaration can have a source location without exposing the interpreter
// that produced its value. This contract makes that provenance available to
// editors and diagnostics through the encountered policy. It neither advances
// a compilation phase nor asks the consumer to supply a native parser object.
//
// get_anchor returns one when it writes an Anchor, or zero when no authored
// location is available. On absence it leaves the output untouched. An Anchor
// identifies the exact borrowed Source observation and byte coordinates.
// Retaining the enclosing publication keeps that observation usable even after
// the host replaces its current source. A zero length range is a real location
// and does not mean that provenance is absent.
typedef struct tetrodotoxin_source_declaration {
  const void* source;
  U8 (*get_anchor)(const void* source, tetrodotoxin_source_anchor* output);
} tetrodotoxin_source_declaration;

PERIMORTEM_C const ttx_representation*
tetrodotoxin_source_declaration_representation(void);

#endif
