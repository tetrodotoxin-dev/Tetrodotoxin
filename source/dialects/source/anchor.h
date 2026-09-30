// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialects/source/token.h"

// An Anchor selects a focus and an inclusive token range in one source
// snapshot. Empty tokens express absent location evidence.
typedef struct tetrodotoxin_source_anchor {
  tetrodotoxin_source_token token;
  tetrodotoxin_source_token start;
  tetrodotoxin_source_token end;
} tetrodotoxin_source_anchor;
