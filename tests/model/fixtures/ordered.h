// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_MODEL_ORDERED_H
#define VALIDATION_MODEL_ORDERED_H

#include "ttx/concept/abstract.h"

// This C provider supplies ordered occurrences through Abstract alone.
// Counters distinguish the property check from binding and show whether a
// terminal visits an unavailable or unreachable sequence.
typedef struct ordered_fixture {
  const ttx_representation* abstract_form;
  const ttx_abstract* children;
  Count count;
  ttx_binding_status ordered_status;
  ttx_binding_status return_status;
  Count visits;
  Count marker_bindings;
} ordered_fixture;

PERIMORTEM_C ttx_abstract ordered_fixture_view(ordered_fixture* fixture);

#endif
