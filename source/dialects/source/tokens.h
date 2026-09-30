// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/data/form/representation.h"

// representation describes one token. data borrows count contiguous records
// within size bytes, including the final Terminal. The supplying Stream keeps
// both the descriptor and immutable payload available for the observation.
typedef struct tetrodotoxin_source_tokens {
  const ttx_representation* representation;
  const U8* data;
  Count size;
  Count count;
} tetrodotoxin_source_tokens;
