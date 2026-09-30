// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/data/form/representation.h"

// The compact profile preserves byte coordinates and a length of at most 255.
// Line and column start at one. Terminal has code zero and length zero.
// A vocabulary identity, supplied by Stream, defines the remaining codes.
typedef struct tetrodotoxin_source_token {
  U16 offset;
  U16 line;
  U16 column;
  U8 size;
  U8 code;
} tetrodotoxin_source_token;

#define TETRODOTOXIN_SOURCE_TERMINAL 0
#define TETRODOTOXIN_SOURCE_UNKNOWN 255
#define TETRODOTOXIN_SOURCE_LEXICON_ID_HIGH 0x83fb72c489814390ULL
#define TETRODOTOXIN_SOURCE_LEXICON_ID_LOW 0xb561a237cc4319d2ULL

PERIMORTEM_C const ttx_representation* tetrodotoxin_source_token_representation(
    void);
