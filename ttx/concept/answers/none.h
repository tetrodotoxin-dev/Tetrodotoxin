// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_ANSWERS_NONE_H
#define TTX_CONCEPT_ANSWERS_NONE_H

#include "ttx/concept/answers/constant.h"

// Completed absence is a marker contract. Binding it to Empty Storage returns
// Satisfied, so consumers recognize absence without comparing addresses or
// acquiring an operation table. ttx_none supplies an Abstract for navigation
// when a graph needs to return that answer as an edge.
#define TTX_NONE_ID_HIGH 0x84c6c053b3254f7aULL
#define TTX_NONE_ID_LOW 0xa873409b31bcf607ULL

PERIMORTEM_C ttx_abstract ttx_none(void);

#endif
