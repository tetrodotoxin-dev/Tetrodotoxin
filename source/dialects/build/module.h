// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_BUILD_MODULE_H
#define TETRODOTOXIN_BUILD_MODULE_H
#include "perimortem/core/view/bytes.h"

// A module has one caller supplied name and native library path. Build loads
// it once for the workspace and discovers roles through TTX negotiation.
// The name selects an importer without guessing which grammar accepts a file.
typedef struct tetrodotoxin_build_module {
  perimortem_view_bytes name;
  perimortem_view_bytes path;
} tetrodotoxin_build_module;
#endif
