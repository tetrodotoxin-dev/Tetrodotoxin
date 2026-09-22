// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_DATA_FORM_COMPOSITION_H
#define VALIDATION_DATA_FORM_COMPOSITION_H

#include "ttx/data/form/representation.h"

PERIMORTEM_C ttx_data_status compose_pair(
    const ttx_representation*, Count, Count, ttx_representation_allocator,
    const ttx_representation**);

#endif
