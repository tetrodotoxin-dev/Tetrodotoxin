// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.h"

// A snapshot lends one source body and its display path. The supplying call or
// retained Stream owns their lifetime. Consumers do not infer ownership from
// either pointer and copy evidence when its supplying scope is too short.
typedef struct tetrodotoxin_source_snapshot {
  perimortem_view_bytes path;
  perimortem_view_bytes text;
} tetrodotoxin_source_snapshot;
