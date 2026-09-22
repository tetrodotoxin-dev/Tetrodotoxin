// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_CONCEPT_ABSTRACT_H
#define TTX_CONCEPT_ABSTRACT_H

#include "perimortem/core/view/bytes.h"
#include "perimortem/core/perimortem.h"

#include "perimortem/system/uuid.h"

#include "ttx/semantic/negotiation/binding.h"

#define TTX_ABSTRACT_ID_HIGH 0x01a08522d86b7decULL
#define TTX_ABSTRACT_ID_LOW 0x9fa65e9c70720964ULL

// A graph can contain providers that have no native C++ Abstract base. Keeping
// the receiver separate from its operations lets those providers answer from
// their own storage, without allocating an adapter object for every graph
// edge. Consumers traverse the supplied operations instead of depending on
// the language or class that implements the subject.
//
// The supplying boundary establishes the exact Abstract contract before the
// table is used, since an unknown table cannot safely describe how to call
// itself. That agreement includes capability queries, interface binding, byte
// observations, resolution and synchronous visitation. This representation uses
// the platform C ABI and has been exercised on Linux with 64 bit pointers.
//
// This contract chooses one opaque receiver and one typed table pointer.
// Its canonical Representation includes the table's callable signatures and
// their recursive Abstract results. Agreement therefore checks more than the
// outer two pointer slots. The receiver's private layout remains unspecified.
//
// The record borrows its provider so graph owners can manage many views under
// one lifetime. Source is nonnull and identifies the subject for that promised
// lifetime, but retaining the token alone keeps neither state nor code alive.
// The caller retains their lifetime owner, including the providers needed by
// returned Abstracts.
typedef struct ttx_abstract {
  const void* source;
  const struct ttx_abstract_ops* operations;
} ttx_abstract;

// A provider can enumerate its own storage directly when the receiver is
// borrowed for just this call. Routes need only survive receive, while values
// keep their ordinary provider lifetime. A consumer that sorts or retains
// routes copies them during the callback. The provider finishes all callbacks
// before visitation returns and cannot retain this receiver for later work.
typedef struct ttx_concept_visitor {
  void* source;
  void (
      *receive)(void* source, perimortem_view_bytes route, ttx_abstract value);
} ttx_concept_visitor;

typedef struct ttx_abstract_ops {
  // Checks if the source provides the contract interface in some form. It
  // doesn't promise any exact representation which is useful for asking
  // semantic questions that don't need the additional overhead of negotating
  // actual storage.
  //
  // It's important to note that binding can still fail due to storage and ABI
  // disagreements even if the contract is supported in another form. Support is
  // a seperate semantic question about capabilities.
  ttx_binding_status (*supports)(const void* source, perimortem_uuid contract);

  // Separating selection from value production lets a consumer obtain an
  // interface without triggering its computation or data transfers. Bind
  // therefore selects an implementation without invoking its value operations.
  // Requested storage describes the complete API the caller can consume.
  // An Abstract request supplies this same view after checking its descriptor.
  ttx_binding_status (*bind)(
      const void* source,
      perimortem_uuid contract,
      ttx_storage requested);

  // The data observation supplies bytes without another Abstract to inspect.
  // They have no implied encoding or Constant promise, even when another edge
  // on this subject is Constant. The view survives until the next observation
  // on this subject or publication release. Retaining it longer requires the
  // enclosing contract's stronger lifetime or a copy made by the consumer.
  perimortem_view_bytes (*get_data)(const void* source);

  // The selected subject brings its own operation table, allowing resolution
  // to cross between different provider representations. It satisfies this
  // same Abstract contract, so the consumer can continue without recovering
  // a native class or reinterpreting the original receiver's storage.
  ttx_abstract (*resolve)(const void* source);

  // Lookup and discovery use the same subject's policy. Visitation advertises
  // the answers visible through that policy and gives their order no meaning.
  // The receiver must not invalidate the traversed state during a callback.
  ttx_abstract (
      *resolve_concept)(const void* source, perimortem_view_bytes route);
  void (*visit_concepts)(const void* source, ttx_concept_visitor visitor);
} ttx_abstract_ops;

PERIMORTEM_C const ttx_representation* ttx_abstract_representation(void);

#endif
