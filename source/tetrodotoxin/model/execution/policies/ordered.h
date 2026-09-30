// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_EXECUTION_POLICIES_ORDERED_H
#define TETRODOTOXIN_MODEL_EXECUTION_POLICIES_ORDERED_H

#include "ttx/concept/abstract.h"

// Ordered gives meaning to this Abstract's visitation sequence. Each callback
// reports the next occurrence in the order defined by the supplying model.
// Repeated subjects remain separate occurrences, and consumers preserve their
// order rather than sorting routes or removing duplicates. Unchanged
// observations preserve that order.
//
// The promise belongs to the encountered policy. A wrapper advertising Ordered
// must preserve its meaning in the visitation it exposes. Ordered alone says
// nothing about executing those subjects, fixing membership forever or caching
// their answers. Those promises belong to the model, Constant or an explicit
// dependency invalidation agreement.
//
// Supports can establish this property without requesting an API record.
// Binding uses an Empty Representation and supplies no operation table.
#define TETRODOTOXIN_MODEL_EXECUTION_ORDERED_ID_HIGH 0x5a3855f4ad5243e7ULL
#define TETRODOTOXIN_MODEL_EXECUTION_ORDERED_ID_LOW 0xb049c60e356b0067ULL

#endif
