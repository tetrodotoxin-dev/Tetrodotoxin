// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/range.h"
#include "ttx/data/form/native.hpp"

namespace Tetrodotoxin::Source {

using Range = tetrodotoxin_source_range;

}  // namespace Tetrodotoxin::Source

TTX_DATA_RECORD(
    tetrodotoxin_source_range,
    TTX_DATA_MEMBER(tetrodotoxin_source_range, offset),
    TTX_DATA_MEMBER(tetrodotoxin_source_range, size));
