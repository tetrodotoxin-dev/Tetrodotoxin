// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/ttx/semantic/fixtures/invocation_provider.h"

#include <string.h>
#include <stddef.h>

#include "ttx/semantic/transport/direct.h"
#include "ttx/semantic/transport/shared.h"
#include "ttx/semantic/transport/block.h"
#include "ttx/semantic/transport/fragment.h"

static struct {
  invocation_statistics statistics;
  ttx_representation record;
  ttx_representation inputs;
  ttx_representation outputs;
  U8 bytes[3][512];
  U8 protocol;
  U8 fail_transfer;
  ttx_binding_status status;
  ttx_invocation invocation;
  R64 bias;
} state;

static ttx_data_status invoke(const void* self, const void* input, void* output) {
  const invocation_input* values = input;
  const R64 bias = *(const R64*)self;
  ++state.statistics.calls;
  R64 result = (values->value + bias) * values->scale;
  *(R64*)output = values->negate ? -result : result;
  return TTX_DATA_SUCCESS;
}

static const ttx_representation* representation(const void* self) {
  (void)self;
  return &state.record;
}

static const void* pointer(const void* self) {
  (void)self;
  return &state.invocation;
}

static ttx_data_status commit(const void* self, ttx_block_surface target) {
  ++state.statistics.transfers;
  if (state.fail_transfer) {
    return TTX_DATA_IO_ERROR;
  }

  // Block creates the bridge in the caller's record. It does not need an
  // independently allocated table for this projected interior receiver.
  const ttx_invocation value = {self, &state.inputs, &state.outputs, invoke};
  memcpy(target.data, &value, sizeof(value));
  return TTX_DATA_SUCCESS;
}

static ttx_data_status get_pointer(const void* self, Count position, void** output) {
  ++state.statistics.transfers;
  if (position == 0) {
    *output = (void*)self;
    return TTX_DATA_SUCCESS;
  }

  if (state.fail_transfer) {
    return TTX_DATA_IO_ERROR;
  }

  if (position == offsetof(ttx_invocation, inputs)) {
    *output = &state.inputs;
    return TTX_DATA_SUCCESS;
  }
  if (position == offsetof(ttx_invocation, outputs)) {
    *output = &state.outputs;
    return TTX_DATA_SUCCESS;
  }

  // The agreed System V carrier contains the executable pointer's bits.
  // Supply that slot separately instead of allocating another combined record.
  ttx_data_status (*operation)(const void*, const void*, void*) = invoke;
  memcpy(output, &operation, sizeof(operation));
  return TTX_DATA_SUCCESS;
}

static void release(const void* self) {
  (void)self;
  ++state.statistics.releases;
}

static ttx_data_status acquire(const void* self, ttx_shared_lifetime* output) {
  (void)self;
  ++state.statistics.acquisitions;
  *output = (ttx_shared_lifetime){&state.invocation, NULL, release};
  return TTX_DATA_SUCCESS;
}

static ttx_binding_status writer(const void* self, perimortem_uuid id,
                                 ttx_storage requested) {
  ++state.statistics.binds;
  if (state.status != TTX_BINDING_SATISFIED) {
    return state.status;
  }

  if (state.protocol == 0 && id.high == TTX_DIRECT_ACCESS_ID_HIGH &&
      id.low == TTX_DIRECT_ACCESS_ID_LOW) {
    static const ttx_direct_access_operations operations = {representation, pointer};
    const ttx_direct_access api = {self, &operations};
    return ttx_binding_provide(ttx_direct_access_representation(), &api, requested);
  }

  if (state.protocol == 1 && id.high == TTX_SHARED_ACCESS_ID_HIGH &&
      id.low == TTX_SHARED_ACCESS_ID_LOW) {
    static const ttx_shared_access_operations operations = {representation, acquire};
    const ttx_shared_access api = {self, &operations};
    return ttx_binding_provide(ttx_shared_access_representation(), &api, requested);
  }

  if (state.protocol == 2 && id.high == TTX_BLOCK_ACCESS_ID_HIGH &&
      id.low == TTX_BLOCK_ACCESS_ID_LOW) {
    static const ttx_block_access_operations operations = {representation, commit};
    const ttx_block_access api = {self, &operations};
    return ttx_binding_provide(ttx_block_access_representation(), &api, requested);
  }

  if (state.protocol == 3 && id.high == TTX_FRAGMENT_ACCESS_ID_HIGH &&
      id.low == TTX_FRAGMENT_ACCESS_ID_LOW) {
    static const ttx_fragment_access_operations operations = {
      .representation = representation,
      .get_pointer = get_pointer,
    };
    const ttx_fragment_access api = {self, &operations};
    return ttx_binding_provide(ttx_fragment_access_representation(), &api, requested);
  }

  return TTX_BINDING_UNSUPPORTED;
}

static invocation_statistics statistics(void) { return state.statistics; }

static ttx_binding_status supports(const void* self, perimortem_uuid id) {
  (void)self;
  if (state.status != TTX_BINDING_SATISFIED) {
    return state.status;
  }

  const perimortem_uuid contracts[] = {
    {TTX_DIRECT_ACCESS_ID_HIGH, TTX_DIRECT_ACCESS_ID_LOW},
    {TTX_SHARED_ACCESS_ID_HIGH, TTX_SHARED_ACCESS_ID_LOW},
    {TTX_BLOCK_ACCESS_ID_HIGH, TTX_BLOCK_ACCESS_ID_LOW},
    {TTX_FRAGMENT_ACCESS_ID_HIGH, TTX_FRAGMENT_ACCESS_ID_LOW}};
  return state.protocol < 4 && id.high == contracts[state.protocol].high &&
                 id.low == contracts[state.protocol].low
             ? TTX_BINDING_SATISFIED : TTX_BINDING_UNSUPPORTED;
}

static void configure(U8 protocol, ttx_binding_status status, U8 fail_transfer) {
  state.protocol = protocol;
  state.status = status;
  state.fail_transfer = fail_transfer;
  state.statistics = (invocation_statistics){0};
}

invocation_fixture invocation_provider_open(
    const ttx_representation* record, const ttx_representation* inputs,
    const ttx_representation* outputs) {
  const ttx_representation* sources[] = {record, inputs, outputs};
  ttx_representation* targets[] = {&state.record, &state.inputs, &state.outputs};
  for (U32 index = 0; index < 3; ++index) {
    if (sources[index]->size > sizeof(state.bytes[index])) {
      return (invocation_fixture){0};
    }

    memcpy(state.bytes[index], sources[index]->data, sources[index]->size);
    *targets[index] = (ttx_representation){state.bytes[index], sources[index]->size};
  }

  state.bias = 2.0;
  state.invocation = (ttx_invocation){&state.bias, &state.inputs, &state.outputs, invoke};
  configure(0, TTX_BINDING_SATISFIED, 0);
  return (invocation_fixture){{&state.bias, writer, supports}, statistics, configure};
}
