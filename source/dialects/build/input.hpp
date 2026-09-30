// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once
#include "tetrodotoxin/dialects/build/input.h"
#include "ttx/concept/abstract.hpp"

TTX_DATA_RECORD(
    tetrodotoxin_build_module,
    TTX_DATA_MEMBER(tetrodotoxin_build_module, name),
    TTX_DATA_MEMBER(tetrodotoxin_build_module, path));
TTX_DATA_RECORD(
    tetrodotoxin_build_source,
    TTX_DATA_MEMBER(tetrodotoxin_build_source, name),
    TTX_DATA_MEMBER(tetrodotoxin_build_source, path),
    TTX_DATA_MEMBER(tetrodotoxin_build_source, importer));
TTX_DATA_RECORD(
    tetrodotoxin_build_input,
    TTX_DATA_MEMBER(tetrodotoxin_build_input, modules),
    TTX_DATA_MEMBER(tetrodotoxin_build_input, module_count),
    TTX_DATA_MEMBER(tetrodotoxin_build_input, sources),
    TTX_DATA_MEMBER(tetrodotoxin_build_input, source_count),
    TTX_DATA_MEMBER(tetrodotoxin_build_input, output),
    TTX_DATA_MEMBER(tetrodotoxin_build_input, reporter),
    TTX_DATA_MEMBER(tetrodotoxin_build_input, report));
