// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_MODULES_MODULE_H
#define TTX_CONCEPT_MODULES_MODULE_H

#include "ttx/concept/abstract.h"
#include "ttx/data/status.h"
#include "ttx/semantic/negotiation/query.h"

// Acquisition supplies the subject promised by a Module. The acquisition
// contract establishes its Abstract interface, so callers can immediately bind
// and navigate the exported subject.
//
// The root can be an interior object or a policy view whose receiver differs
// from the allocation owner. Release therefore takes a separate owner pointer.
// Success transfers this release obligation. Every borrowed root or descendant
// must finish before release, and the supplying Module must remain alive through
// that release. An emitted factory can retain the Module after acquisition ends.
typedef struct ttx_module_acquisition {
  ttx_abstract root;
  const void* owner;
  void (*release)(const void* owner);
} ttx_module_acquisition;

// The native entry is the same acquisition agreement an embedded Module can
// provide. Host services arrive through an already established Query and remain
// borrowed through every result that uses them. Failure transfers nothing.
// The module and loader agree on the target's native C ABI before this entry
// runs. That bootstrap lets the loader receive its first subject before asking
// the subject to negotiate further interfaces.
// Only this Concept acquisition protocol promises an Abstract. Independent
// Semantic endpoints can continue to supply Query without a Concept surface.
typedef ttx_data_status (*ttx_module_entry)(
    ttx_semantic_query host, ttx_module_acquisition* output);

// A Module owns an acquisition implementation. Native implementations retain
// their executable library, while embedded implementations may retain managed
// state instead. Copies retain that implementation independently of each opened
// acquisition, allowing source graphs to be discarded before runtime execution.
typedef struct ttx_module {
  const void* source;
  void (*retain)(const void* source);
  void (*release)(const void* source);
  ttx_data_status (*open)(
      const void* source,
      ttx_semantic_query host,
      ttx_module_acquisition* output);
} ttx_module;

#define TTX_MODULE_ENTRY "ttx_module_open"

#endif
