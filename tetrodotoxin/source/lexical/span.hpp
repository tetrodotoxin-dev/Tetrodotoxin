// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/lexical/span.h"
#include "tetrodotoxin/source/lexical/token.hpp"

namespace Tetrodotoxin::Source::Lexical {

using Span = tetrodotoxin_source_span;

static_assert(sizeof(Span) == 16);

}  // namespace Tetrodotoxin::Source::Lexical

TTX_DATA_RECORD(
    tetrodotoxin_source_span,
    TTX_DATA_MEMBER(tetrodotoxin_source_span, start),
    TTX_DATA_MEMBER(tetrodotoxin_source_span, end));
