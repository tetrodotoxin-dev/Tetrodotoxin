// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_CONCEPT_OBSERVATION_H
#define VALIDATION_CONCEPT_OBSERVATION_H

#include "ttx/concept/abstract.h"

typedef struct observation_subject {
  const ttx_representation* abstract_form;
  const ttx_representation* marker_form;
  U8 value;
} observation_subject;

PERIMORTEM_C ttx_abstract observation_abstract(observation_subject* subject);

#endif
