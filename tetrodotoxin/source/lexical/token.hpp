// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/lexical/token.h"
#include "ttx/data/form/native.hpp"

namespace Tetrodotoxin::Source::Lexical {

using Token = tetrodotoxin_source_token;

static_assert(sizeof(Token) == 8);

}  // namespace Tetrodotoxin::Source::Lexical

TTX_DATA_RECORD(
    tetrodotoxin_source_token,
    TTX_DATA_MEMBER(tetrodotoxin_source_token, value));
