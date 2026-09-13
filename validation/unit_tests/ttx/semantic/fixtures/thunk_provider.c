// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/ttx/semantic/fixtures/thunk_provider.h"

#include <string.h>

static struct {
  counter_statistics statistics;
  ttx_binding_status query_status;
  ttx_binding_status fulfillment_status;
  U8 omit;
  U8 stateless;
  U64 counter;
  U8 bytes[64];
  ttx_representation representation;
} state;

static U64 add(const void* self, U64 amount) {
  ++state.statistics.calls;
  if (!self) {
    return amount;
  }

  U64* counter = (U64*)self;
  *counter += amount;
  return *counter;
}

static ttx_binding_status fulfill(
    const void* self, perimortem_uuid contract,
    ttx_calling_convention convention, const ttx_representation* representation,
    ttx_binding* output) {
  (void)self;
  ++state.statistics.fulfillments;
  if (state.fulfillment_status != TTX_BINDING_SATISFIED || state.omit) {
    return state.fulfillment_status;
  }

  if (contract.high != COUNTER_ID_HIGH || contract.low != COUNTER_ID_LOW ||
      convention != TTX_CALLING_SYSTEM_V_AMD64) {
    return TTX_BINDING_UNSUPPORTED;
  }

  if (representation->size != state.representation.size ||
      memcmp(representation->data, state.bytes, representation->size)) {
    return TTX_BINDING_REJECTED;
  }

  static const counter_operations operations = {add};
  // This receiver is an interior field, not the original Query's subject.
  // The C++ caller has no right to reconstruct that enclosing native struct.
  output->source = state.stateless ? NULL : &state.counter;
  output->operations = &operations;
  return TTX_BINDING_SATISFIED;
}

static ttx_binding_status bind(
    const void* self, perimortem_uuid contract, ttx_binding* output) {
  ++state.statistics.queries;
  if (state.query_status != TTX_BINDING_SATISFIED) {
    return state.query_status;
  }

  if (contract.high != TTX_THUNK_ID_HIGH || contract.low != TTX_THUNK_ID_LOW) {
    return TTX_BINDING_UNSUPPORTED;
  }

  static const ttx_thunk_operations operations = {fulfill};
  output->source = self;
  output->operations = &operations;
  return TTX_BINDING_SATISFIED;
}

static void reset(ttx_binding_status query, ttx_binding_status fulfillment,
                  U8 omit, U8 stateless) {
  state.statistics = (counter_statistics){0, 0, 0};
  state.counter = 0;
  state.query_status = query;
  state.fulfillment_status = fulfillment;
  state.omit = omit;
  state.stateless = stateless;
}

static counter_statistics statistics(void) {
  return state.statistics;
}

counter_fixture thunk_provider_open(const ttx_representation* representation) {
  // This fixture tests independently compiled C dispatch, not an independent
  // canonicalizer. Its own copy ensures byte agreement cannot use pointer
  // equality with the host's constexpr publication.
  if (representation->size > sizeof(state.bytes)) {
    return (counter_fixture){0};
  }

  memcpy(state.bytes, representation->data, representation->size);
  state.representation = (ttx_representation){state.bytes, representation->size};
  reset(TTX_BINDING_SATISFIED, TTX_BINDING_SATISFIED, 0, 0);
  return (counter_fixture){{&state, bind}, reset, statistics};
}
