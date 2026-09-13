// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_SEMANTIC_THUNK_H
#define TTX_SEMANTIC_THUNK_H

#include "perimortem/system/uuid.h"

#include "ttx/data/form/representation.h"
#include "ttx/semantic/binding.h"

// UUID binding establishes which questions an interface answers through a
// mutually negotiated data layer, but before a foreign consumer can call its
// functions the provider must _also_ agree to their concrete realization.
//
// Thunk requests an operation table using the calling convention and prepared
// representation supplied by that consumer. The contract UUID specifies every
// signature, including the explicit receiver, argument and result meanings.
// Representation describes the table's storage where equal pointer slots alone
// do not establish equal callable signatures.
//
// A successful provider supplies matching executable thunks and their opaque
// receiver as one Binding. It may return a projection rather than its original
// object. Calls are explicitly synchronous for the boundary so various RPC
// implementations can perform adaptation behind these thunks while local uses
// don't have to pay the over head of async wrappers (unless they want to).
//
// The resulting publication owns the receiver, table and executable code for
// every use of this borrowed interface. Providers requiring a shorter lifetime
// must expose that ownership in their enclosing contract. Copying the pair
// cannot extend a module's lifetime or acquire a device allocation.
#define TTX_THUNK_ID_HIGH ((U64)0xb56631c2486248a7ULL)
#define TTX_THUNK_ID_LOW ((U64)0x98024dd06a8022d1ULL)

typedef U32 ttx_calling_convention;
#define TTX_CALLING_SYSTEM_V_AMD64 ((ttx_calling_convention)1)

typedef struct ttx_thunk_operations {
  ttx_binding_status (*fulfill)(
      const void* source,
      perimortem_uuid contract,
      ttx_calling_convention convention,
      const ttx_representation* representation,
      ttx_binding* output);
} ttx_thunk_operations;

#endif
