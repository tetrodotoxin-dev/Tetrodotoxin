// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_DECLARATIONS_CALLABLE_H
#define TTX_CONCEPT_DECLARATIONS_CALLABLE_H

#include "ttx/concept/abstract.h"
#include "ttx/data/form/representation.h"

#define TTX_CALLABLE_ID_HIGH 0x84c6c053b3254f7aULL
#define TTX_CALLABLE_ID_LOW 0xa873409b31bcf605ULL

// A terminal needs both the concrete argument storage and the questions each
// argument answers. Identical bytes cannot distinguish a Boolean from a byte,
// or text from an arbitrary borrowed region. Each field therefore supplies its
// encountered Abstract and its placement in the frame. The terminal negotiates
// the meanings it understands without a universal enumeration of value kinds.
typedef struct ttx_callable_field {
  ttx_abstract subject;
  Count offset;
} ttx_callable_field;

typedef struct ttx_callable_frame {
  const ttx_representation* representation;
  const ttx_callable_field* fields;
  Count count;
} ttx_callable_frame;

// Callable describes an operation so a terminal can prepare its arguments and
// results during discovery. Its UUID identifies the operation the runtime
// instance supplies through Semantic Invocation.
// Input and output frames describe that invocation's payload. The opaque self
// pointer is an explicit argument of the invocation ABI, outside those frames.
// An empty frame means no payload and requires no storage.
//
// Description, field Abstracts and representations borrow the discovery
// publication. A terminal copies its required facts before releasing that
// graph. Runtime fulfillment must succeed without consulting it again.
// Borrowed inputs remain readable through synchronous result conversion.
// Borrowed outputs may reference those inputs or instance storage and remain
// readable until conversion completes, before another call mutates the owner.
typedef struct ttx_callable_description {
  perimortem_uuid contract;
  ttx_callable_frame inputs;
  ttx_callable_frame outputs;
} ttx_callable_description;

typedef struct ttx_callable_operations {
  ttx_binding_status (*describe)(const void* source,
                                 ttx_callable_description* output);
} ttx_callable_operations;

typedef struct ttx_callable {
  const void* source;
  const ttx_callable_operations* operations;
} ttx_callable;

PERIMORTEM_C const ttx_representation* ttx_callable_representation(void);

#endif
