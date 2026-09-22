// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_ANSWERS_UNKNOWN_H
#define TTX_CONCEPT_ANSWERS_UNKNOWN_H

#include "ttx/concept/abstract.h"

// Unknown is a provisional answer. Its marker binding to Empty Storage returns
// Satisfied, while unsettled capability questions return Pending. Binding
// Abstract still supplies the existing navigation view, allowing callers to
// keep the graph route without confusing provisional state with absence.
#define TTX_UNKNOWN_ID_HIGH 0x84c6c053b3254f7aULL
#define TTX_UNKNOWN_ID_LOW 0xa873409b31bcf608ULL

PERIMORTEM_C ttx_abstract ttx_unknown(void);

#endif
