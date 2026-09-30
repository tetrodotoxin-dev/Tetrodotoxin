// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_BUILD_WORKSPACE_H
#define TETRODOTOXIN_BUILD_WORKSPACE_H
#include "ttx/concept/abstract.h"

#define TETRODOTOXIN_BUILD_WORKSPACE_ID_HIGH 0x1f142e17c74444bcULL
#define TETRODOTOXIN_BUILD_WORKSPACE_ID_LOW 0xa1441535643204ecULL

// Workspace owns the imported roots and the modules supplying their code.
// Abstract routes are the exact declared source names. Navigation lends the
// real roots and creates no acquisition. Terminals negotiate their own domain
// contracts on those roots and may refuse observations they cannot produce.
//
// Borrow acquires the entire workspace, keeping both source data and provider
// code alive. A caller retaining a child beyond the workspace observation also
// retains the workspace. Child reservations must be released before that final
// workspace reservation. All acquisitions remain on the owning worker.
typedef struct tetrodotoxin_build_workspace_ops {
  ttx_abstract_ops abstract;
  perimortem_view_bytes (*output_directory)(const void* source);
} tetrodotoxin_build_workspace_ops;

typedef struct tetrodotoxin_build_workspace {
  const void* source;
  const tetrodotoxin_build_workspace_ops* operations;
} tetrodotoxin_build_workspace;

PERIMORTEM_C const ttx_representation*
    tetrodotoxin_build_workspace_representation(void);
#endif
