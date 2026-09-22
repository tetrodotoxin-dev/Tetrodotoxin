// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_DOMAIN_H
#define TTX_CONCEPT_DOMAIN_H

#include "ttx/concept/abstract.h"

#define TTX_DOMAIN_ID_HIGH 0x01a087d16837717bULL
#define TTX_DOMAIN_ID_LOW 0xb181cda6355d7b53ULL

// A produced value and the domain used to interpret it answer different
// questions. This operation supplies their relationship without replacing the
// producer or requiring it to be a memory location. Its returned Abstract
// preserves the authority through which the domain is reached.
//
// The provider owns that edge and its policy. It writes the borrowed result
// only on Satisfied. Unsupported, Pending and Rejected retain their binding
// meanings, so a consumer cannot bypass an unfinished or restricted answer.
typedef struct ttx_domain_ops {
  ttx_binding_status (*get_domain)(const void* source, ttx_abstract* result);
} ttx_domain_ops;

typedef struct ttx_domain {
  const void* source;
  const ttx_domain_ops* operations;
} ttx_domain;

#endif
