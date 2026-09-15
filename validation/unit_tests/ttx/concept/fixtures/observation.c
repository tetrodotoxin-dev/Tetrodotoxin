// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/ttx/concept/fixtures/observation.h"
#include "ttx/concept/answers/constant.h"
#include "ttx/concept/answers/none.h"

static ttx_binding_status bind(
    const void* source, perimortem_uuid id, ttx_storage output) {
  const observation_subject* subject = source;
  if (id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW) {
    const ttx_abstract api = observation_abstract((observation_subject*)source);
    return ttx_binding_provide(subject->abstract_form, &api, output);
  }
  if (id.high == TTX_CONSTANT_ID_HIGH && id.low == TTX_CONSTANT_ID_LOW) {
    return ttx_binding_provide(subject->marker_form, 0, output);
  }
  return TTX_BINDING_UNSUPPORTED;
}

// Constant describes the subject's semantic answers. It does not make its
// byte observation a constant, nor extend the returned buffer's lifetime.
static perimortem_view_bytes data(const void* source) {
  observation_subject* subject = (observation_subject*)source;
  ++subject->value;
  const perimortem_view_bytes bytes = {&subject->value, 1};
  return bytes;
}

static ttx_abstract resolve(const void* source) {
  return observation_abstract((observation_subject*)source);
}

static ttx_abstract lookup(const void* source, perimortem_view_bytes route) {
  if (route.size == 3 && route.data[0] == 7 && route.data[1] == 0 && route.data[2] == 9) {
    return resolve(source);
  }
  return ttx_none();
}

static void visit(const void* source, ttx_concept_visitor visitor) {
  const U8 route[] = {7, 0, 9};
  const perimortem_view_bytes bytes = {route, sizeof(route)};
  visitor.receive(visitor.source, bytes, resolve(source));
}

static U8 satisfies(const void* source, ttx_abstract requirement) {
  (void)source;
  const perimortem_view_bytes wanted = requirement.operations->get_data(requirement.source);
  return wanted.size == 1 && wanted.data[0] == 42;
}

ttx_abstract observation_abstract(observation_subject* subject) {
  static const ttx_abstract_ops operations = {bind, data, resolve, lookup, visit, satisfies};
  const ttx_abstract result = {subject, &operations};
  return result;
}
