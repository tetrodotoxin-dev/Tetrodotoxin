// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tests/model/fixtures/ordered.h"

#include "tetrodotoxin/model/execution/return.h"
#include "ttx/concept/answers/none.h"
#include "tetrodotoxin/model/execution/policies/ordered.h"

static ttx_binding_status supports(const void* source, perimortem_uuid id) {
  const ordered_fixture* fixture = source;
  if (id.high == TETRODOTOXIN_MODEL_EXECUTION_ORDERED_ID_HIGH && id.low == TETRODOTOXIN_MODEL_EXECUTION_ORDERED_ID_LOW) {
    return fixture->ordered_status;
  }
  return id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW
             ? TTX_BINDING_SATISFIED
             : TTX_BINDING_UNKNOWN;
}

static ttx_binding_status bind(
    const void* source, perimortem_uuid id, ttx_storage destination) {
  ordered_fixture* fixture = (ordered_fixture*)source;
  if (id.high == TETRODOTOXIN_MODEL_EXECUTION_ORDERED_ID_HIGH && id.low == TETRODOTOXIN_MODEL_EXECUTION_ORDERED_ID_LOW) {
    ++fixture->marker_bindings;
    return TTX_BINDING_REJECTED;
  }
  if (id.high == TETRODOTOXIN_MODEL_EXECUTION_RETURN_ID_HIGH &&
      id.low == TETRODOTOXIN_MODEL_EXECUTION_RETURN_ID_LOW) {
    return fixture->return_status;
  }
  if (id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW) {
    const ttx_abstract api = ordered_fixture_view(fixture);
    return ttx_binding_provide(fixture->abstract_form, &api, destination);
  }
  return TTX_BINDING_UNKNOWN;
}

static perimortem_view_bytes data(const void* source) {
  (void)source;
  return (perimortem_view_bytes){0, 0};
}

static ttx_abstract resolve(const void* source) {
  return ordered_fixture_view((ordered_fixture*)source);
}

static ttx_abstract lookup(const void* source, perimortem_view_bytes route) {
  const ordered_fixture* fixture = source;
  if (route.size == 1 && route.data[0] < fixture->count) {
    return fixture->children[fixture->count - 1 - route.data[0]];
  }
  return ttx_none();
}

static void visit(const void* source, ttx_concept_visitor visitor) {
  ordered_fixture* fixture = (ordered_fixture*)source;
  ++fixture->visits;
  for (Count index = 0; index < fixture->count; ++index) {
    // Descending routes separate visitation order from route sorting. The
    // fixture uses fewer than 256 entries, with no shared native index format.
    const U8 route = (U8)(fixture->count - 1 - index);
    const perimortem_view_bytes key = {&route, 1};
    visitor.receive(visitor.source, key, fixture->children[index]);
  }
}

ttx_abstract ordered_fixture_view(ordered_fixture* fixture) {
  static const ttx_abstract_ops operations = {
    supports, bind, data, resolve, lookup, visit,
  };
  return (ttx_abstract){fixture, &operations};
}
