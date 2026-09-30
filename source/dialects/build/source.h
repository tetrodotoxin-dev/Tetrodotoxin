// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_BUILD_SOURCE_H
#define TETRODOTOXIN_BUILD_SOURCE_H
#include "perimortem/core/view/bytes.h"

// One workspace edge names a source independently of its physical input path.
// Importer names a declared module. The resulting edge reaches that provider's
// real Abstract, with no reconstructed declaration or normalized syntax tree.
typedef struct tetrodotoxin_build_source {
  perimortem_view_bytes name;
  perimortem_view_bytes path;
  perimortem_view_bytes importer;
} tetrodotoxin_build_source;
#endif
