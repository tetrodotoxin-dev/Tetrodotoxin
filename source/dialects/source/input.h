// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.h"

// Source input names an immutable byte observation through its real owner.
// Import first negotiates Borrow on text. Successful acquisition supplies the
// data view and stays retained for the resulting Stream's lifetime. Unknown or
// Rejected at either negotiation step makes Import copy text's current data.
// The caller lends this input and path during Import. The path is copied.
//
// A provider lending source bytes keeps the acquired data view and its contents
// unchanged until release. Borrow supplies storage lifetime, not a general
// promise that every Abstract is immutable. Supplying code remains loaded while
// Source can invoke the acquired answer's operations, including release.
typedef struct tetrodotoxin_source_input {
  perimortem_view_bytes path;
  ttx_abstract text;
} tetrodotoxin_source_input;

PERIMORTEM_C const ttx_representation* tetrodotoxin_source_input_representation(
    void);
