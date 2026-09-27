// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_MODEL_FOREIGN_H
#define VALIDATION_MODEL_FOREIGN_H

#include "tetrodotoxin/model/type/storage.h"
#include "ttx/concept/domain.h"
#include "tetrodotoxin/model/type/policies/conversion.h"
#include "tetrodotoxin/model/type/policies/unsigned.h"
#include "tetrodotoxin/model/execution/function.h"
#include "tetrodotoxin/model/execution/field.h"
#include "tetrodotoxin/model/execution/parameter.h"
#include "tetrodotoxin/model/execution/constant.h"
#include "tetrodotoxin/model/execution/value.h"
#include "tetrodotoxin/model/execution/return.h"
#include "ttx/data/protocol/block.h"
#include "ttx/semantic/transport/block.h"

// The fixture borrows the host's canonical API descriptions. It independently
// implements the entire model in C, including a Block supplied constant. This
// tests foreign semantic composition, not an independent schema compiler.
typedef struct model_forms {
  const ttx_representation* abstract;
  const ttx_representation* storage;
  const ttx_representation* type_storage;
  const ttx_representation* field;
  const ttx_representation* parameter;
  const ttx_representation* returned;
  const ttx_representation* function;
  const ttx_representation* block;
  const ttx_representation* domain;
  const ttx_representation* conversion;
  const ttx_representation* value;
} model_forms;

typedef struct model_node {
  void* owner;
  U8 kind;
  ttx_abstract subject;
} model_node;

typedef struct model_fixture {
  model_forms forms;
  const ttx_representation* payload;
  model_node nodes[7];
  U8 mode;
  U8 alive;
  U8 wrong_parameter;
  ttx_binding_status refusal;
  Count bindings;
  Count observations;
  Count reads;
  U32 literal;
} model_fixture;

PERIMORTEM_C ttx_abstract model_fixture_open(model_fixture* fixture, model_forms forms, U8 mode);

#endif
