// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_BUILD_INPUT_H
#define TETRODOTOXIN_BUILD_INPUT_H
#include "tetrodotoxin/dialects/build/module.h"
#include "tetrodotoxin/dialects/build/source.h"
#include "ttx/data/form/representation.h"

// The native Build seed accepts an explicit module inventory and source edges.
// These records describe the build without choosing an authored Build grammar.
// Names are unique within each inventory. Paths are native filesystem inputs
// and cannot contain embedded zero bytes. Output names a directory for
// terminals.
//
// The caller lends all records, views and the optional report callback through
// Import.visit. Report messages survive only their callback. A successful build
// publishes its Workspace after every input and every discovered terminal has
// accepted the request. Terminals own their artifacts and their own publication
// policy, so failure does not promise rollback of earlier terminal effects.
typedef struct tetrodotoxin_build_input {
  const tetrodotoxin_build_module* modules;
  Count module_count;
  const tetrodotoxin_build_source* sources;
  Count source_count;
  perimortem_view_bytes output;
  void* reporter;
  void (*report)(void* reporter, perimortem_view_bytes message);
} tetrodotoxin_build_input;

PERIMORTEM_C const ttx_representation* tetrodotoxin_build_input_representation(
    void);
#endif
