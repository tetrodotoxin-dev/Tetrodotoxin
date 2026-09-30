// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialects/source/input.h"
#include "ttx/concept/abstract.hpp"

TTX_DATA_RECORD(
    tetrodotoxin_source_input,
    TTX_DATA_MEMBER(tetrodotoxin_source_input, path),
    TTX_DATA_MEMBER(tetrodotoxin_source_input, text));
