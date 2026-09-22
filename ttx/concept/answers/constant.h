// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_ANSWERS_CONSTANT_H
#define TTX_CONCEPT_ANSWERS_CONSTANT_H

#include "ttx/concept/abstract.h"

// Binding this contract to an Empty Representation and receiving Satisfied
// proves that the observed resolution edge is final. No operation table is
// supplied, and this promise does not make every outgoing edge Constant.
// Native and foreign consumers can therefore establish the same proof while
// keeping their own object representations.
#define TTX_CONSTANT_ID_HIGH 0x84c6c053b3254f7aULL
#define TTX_CONSTANT_ID_LOW 0xa873409b31bcf606ULL

#endif
