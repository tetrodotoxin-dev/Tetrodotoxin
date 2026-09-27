// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/anchor.hpp"
#include "tetrodotoxin/source/diagnostic.h"

namespace Tetrodotoxin::Source {
using Diagnostic = tetrodotoxin_source_diagnostic;
}  // namespace Tetrodotoxin::Source

TTX_DATA_RECORD(
    tetrodotoxin_source_diagnostic,
    TTX_DATA_MEMBER(tetrodotoxin_source_diagnostic, anchor),
    TTX_DATA_MEMBER(tetrodotoxin_source_diagnostic, message),
    TTX_DATA_MEMBER(tetrodotoxin_source_diagnostic, hint),
    TTX_DATA_MEMBER(tetrodotoxin_source_diagnostic, has_anchor));
