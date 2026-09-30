// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialects/source/stream.h"

// The caller owns this call state. begin and end bound a token range, with end
// excluded from authored tokens and no later than the Stream's Terminal.
// Parsers may advance or rewind index within that range. They do not replace
// the stream or bounds. Nested Parse calls share this same position.
// No allocation, diagnostic sink or semantic result is owned by Cursor.
typedef struct tetrodotoxin_source_cursor {
  tetrodotoxin_source_stream stream;
  U64 begin;
  U64 index;
  U64 end;
} tetrodotoxin_source_cursor;
