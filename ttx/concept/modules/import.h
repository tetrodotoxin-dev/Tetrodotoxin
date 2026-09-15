// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_MODULES_IMPORT_H
#define TTX_CONCEPT_MODULES_IMPORT_H

#include "perimortem/core/view/bytes.h"

#include "ttx/concept/modules/module.h"

#define TTX_IMPORT_ID_HIGH 0x84c6c053b3254f7aULL
#define TTX_IMPORT_ID_LOW 0xa873409b31bcf601ULL

// Import resolves an identifier chosen by the host to an owned Module policy.
// Files, deployment paths and caches stay with that host. A provider can ask
// for "cpu" without knowing whether its caller uses Godot, a CLI or a build
// tool. Opening the returned Module acquires an Abstract with its own lifetime.
// The service is synchronous, borrows names for the call, and writes output
// only on success. Its owner outlives every user of this borrowed service.
typedef struct ttx_import_operations {
  ttx_data_status (*open)(
      const void* source,
      perimortem_view_bytes name,
      ttx_module* output);
} ttx_import_operations;

typedef struct ttx_import {
  const void* source;
  const ttx_import_operations* operations;
} ttx_import;

PERIMORTEM_C const ttx_representation* ttx_import_representation(void);

#endif
