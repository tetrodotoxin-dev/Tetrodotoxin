// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/ttx/semantic/fixtures/interface_provider.h"
#include "validation/unit_tests/ttx/semantic/fixtures/provider_representation.h"

#include <stddef.h>

static struct {
  U64 guard;
  U64 value;
  counter_statistics statistics;
  ttx_binding_status status;
  U8 omit;
  U8 stateless;
} state;
static const ttx_representation* representation;

static U64 add(const void* receiver, U64 amount) {
  ++state.statistics.calls;
  if (!receiver) {
    return amount;
  }

  U64* value = (U64*)receiver;
  *value += amount;
  return *value;
}

static U64 read(const void* receiver) {
  ++state.statistics.calls;
  return receiver ? *(const U64*)receiver : 0;
}

static ttx_binding_status bind(
    const void* source, perimortem_uuid id, ttx_storage requested) {
  (void)source;
  ++state.statistics.queries;
  if (state.status != TTX_BINDING_SATISFIED) {
    return state.status;
  }

  if (id.high != COUNTER_ID_HIGH || id.low != COUNTER_ID_LOW) {
    return TTX_BINDING_UNSUPPORTED;
  }

  if (state.omit) {
    return TTX_BINDING_SATISFIED;
  }

  const counter_api api = {state.stateless ? NULL : &state.value, add, read};
  return ttx_binding_provide(representation, &api, requested);
}

static void reset(ttx_binding_status status, U8 omit, U8 stateless) {
  state.statistics = (counter_statistics){0};
  state.value = 0;
  state.status = status;
  state.omit = omit;
  state.stateless = stateless;
}

// Support observes the semantic promise without constructing the counter API.
// A caller can keep using this fact even if its callable layout has drifted.
static ttx_binding_status supports(const void* source, perimortem_uuid id) {
  (void)source;
  if (state.status != TTX_BINDING_SATISFIED) {
    return state.status;
  }

  return id.high == COUNTER_ID_HIGH && id.low == COUNTER_ID_LOW
             ? TTX_BINDING_SATISFIED : TTX_BINDING_UNSUPPORTED;
}

static counter_statistics statistics(void) { return state.statistics; }

counter_fixture interface_provider_open(interface_compile compiler) {
  // These are independently authored C declarations. The host supplies only
  // the compiler, not the expected interface descriptor or native operation
  // identities. sizeof/offsetof retain the actual C record's geometry.
  static const ttx_schema integer = {
    8, 8, TTX_SCHEMA_VALUE, {.value = {TTX_SCHEMA_U64, TTX_SCHEMA_LITTLE_ENDIAN}}};
  static const ttx_schema_argument arguments[] = {
    {{NULL, TTX_SCHEMA_REFERENCE_POINTER}, 1}, {{&integer, 0}, 1}};
  static const ttx_schema addition = {
    8, 8, TTX_SCHEMA_CALLABLE,
    {.callable = {arguments, 2, {&integer, 0}, TTX_SCHEMA_SYSTEM_V_AMD64}}};
  static const ttx_schema observation = {
    8, 8, TTX_SCHEMA_CALLABLE,
    {.callable = {arguments, 1, {&integer, 0}, TTX_SCHEMA_SYSTEM_V_AMD64}}};
  static const ttx_schema_position fields[] = {
    {{NULL, TTX_SCHEMA_REFERENCE_POINTER}, offsetof(counter_api, receiver)},
    {{&addition, 0}, offsetof(counter_api, add)},
    {{&observation, 0}, offsetof(counter_api, read)}};
  static const ttx_schema schema = {
    sizeof(counter_api), _Alignof(counter_api), TTX_SCHEMA_COMPOSITE,
    {.composite = {fields, 3}}};
  compile_representation = compiler;
  if (!representation) {
    representation = prepare_representation(&schema);
  }

  reset(TTX_BINDING_SATISFIED, 0, 0);
  return (counter_fixture){{&state, bind, supports}, reset, statistics};
}
