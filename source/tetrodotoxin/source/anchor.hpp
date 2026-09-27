// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/anchor.h"
#include "tetrodotoxin/source/range.hpp"

namespace Tetrodotoxin::Source {

using Anchor = tetrodotoxin_source_anchor;

}  // namespace Tetrodotoxin::Source

TTX_DATA_RECORD(
    tetrodotoxin_source_anchor,
    TTX_DATA_MEMBER(tetrodotoxin_source_anchor, source),
    TTX_DATA_MEMBER(tetrodotoxin_source_anchor, extent),
    TTX_DATA_MEMBER(tetrodotoxin_source_anchor, focus),
    TTX_DATA_MEMBER(tetrodotoxin_source_anchor, has_focus));
