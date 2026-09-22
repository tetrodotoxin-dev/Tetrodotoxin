// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_SOURCE_RANGE_H
#define TETRODOTOXIN_SOURCE_RANGE_H

#include "perimortem/core/perimortem.h"

// Source locations use byte coordinates so their meaning survives a change of
// tokenizer. A range selects size bytes starting at offset. Zero size is a
// valid insertion point, including the position just after the last byte.
typedef struct tetrodotoxin_source_range {
  U64 offset;
  U64 size;

#ifdef __cplusplus
  constexpr tetrodotoxin_source_range(U64 offset = 0, U64 size = 0)
      : offset(offset), size(size) {}
  constexpr auto get_offset() const -> U64 { return offset; }
  constexpr auto get_size() const -> U64 { return size; }
#endif
} tetrodotoxin_source_range;

#endif
