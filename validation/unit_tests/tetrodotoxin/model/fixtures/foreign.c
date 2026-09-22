// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/tetrodotoxin/model/fixtures/foreign.h"

#include <stdlib.h>
#include <string.h>
#include "ttx/concept/answers/none.h"

enum { TYPE, INPUT, OUTPUT, PARAMETER, CONSTANT, RETURN, FUNCTION };

static model_fixture* observe(const void* source) {
  model_fixture* fixture = ((const model_node*)source)->owner;
  if (!fixture->alive) { abort(); }
  ++fixture->observations;
  return fixture;
}

static U8 equal(perimortem_uuid id, U64 high, U64 low) {
  return id.high == high && id.low == low;
}

static ttx_abstract type(const void* source) {
  return observe(source)->nodes[TYPE].subject;
}

static ttx_binding_status storage(const void* source, const ttx_representation** output) {
  *output = observe(source)->forms.storage;
  return TTX_BINDING_SATISFIED;
}

static ttx_binding_status domain(const void* source, ttx_abstract* output) {
  *output = type(source);
  return TTX_BINDING_SATISFIED;
}

// This C receiver performs the same current value question as a native Type.
// It follows the source's Domain policy instead of treating a previous width
// answer or native class as permission for a later conversion.
static ttx_binding_status convert(const void* source, ttx_abstract value, ttx_abstract* output) {
  model_fixture* fixture = observe(source);
  const perimortem_uuid domain_id = {TTX_DOMAIN_ID_HIGH, TTX_DOMAIN_ID_LOW};
  ttx_domain actual_domain;
  ttx_storage target = {fixture->forms.domain, (U8*)&actual_domain, sizeof(actual_domain)};
  ttx_binding_status status = value.operations->bind(value.source, domain_id, target);
  if (status != TTX_BINDING_SATISFIED) { return status; }

  ttx_abstract source_type;
  status = actual_domain.operations->get_domain(actual_domain.source, &source_type);
  if (status != TTX_BINDING_SATISFIED) { return status; }

  const perimortem_uuid unsigned_id = {TETRODOTOXIN_MODEL_TYPE_UNSIGNED_ID_HIGH, TETRODOTOXIN_MODEL_TYPE_UNSIGNED_ID_LOW};
  status = source_type.operations->supports(source_type.source, unsigned_id);
  if (status != TTX_BINDING_SATISFIED) { return status; }

  const perimortem_uuid storage_id = {TETRODOTOXIN_MODEL_TYPE_STORAGE_ID_HIGH, TETRODOTOXIN_MODEL_TYPE_STORAGE_ID_LOW};
  tetrodotoxin_model_type_storage actual_storage;
  target = (ttx_storage){fixture->forms.type_storage, (U8*)&actual_storage, sizeof(actual_storage)};
  status = source_type.operations->bind(source_type.source, storage_id, target);
  if (status != TTX_BINDING_SATISFIED) { return status; }

  const ttx_representation* form;
  status = actual_storage.get_representation(actual_storage.source, &form);
  if (status != TTX_BINDING_SATISFIED) { return status; }
  if (!ttx_representation_compatible(form, fixture->forms.storage)) { return TTX_BINDING_REJECTED; }

  *output = value;
  return TTX_BINDING_SATISFIED;
}

static Count size(const void* source) {
  model_fixture* fixture = observe(source);
  const U8 kind = ((const model_node*)source)->kind;
  return fixture->mode == 2 || (kind == INPUT && fixture->mode == 1) ? 0 : 1;
}

static ttx_abstract at(const void* source, Count index) {
  (void)index;
  model_fixture* fixture = observe(source);
  const U8 kind = ((const model_node*)source)->kind;
  if (kind == RETURN) {
    return fixture->nodes[fixture->mode == 1 ? CONSTANT : PARAMETER].subject;
  }
  return fixture->nodes[kind].subject;
}

static tetrodotoxin_model_execution_layout parameters(const void* source) {
  model_fixture* fixture = observe(source);
  const tetrodotoxin_model_execution_layout layout = {&fixture->nodes[INPUT], size, at};
  return layout;
}

static tetrodotoxin_model_execution_layout results(const void* source) {
  model_fixture* fixture = observe(source);
  const tetrodotoxin_model_execution_layout layout = {&fixture->nodes[OUTPUT], size, at};
  return layout;
}

static tetrodotoxin_model_execution_layout values(const void* source) {
  observe(source);
  const tetrodotoxin_model_execution_layout layout = {source, size, at};
  return layout;
}

static ttx_abstract body(const void* source) { return observe(source)->nodes[RETURN].subject; }

static ttx_abstract parameter(const void* source) {
  model_fixture* fixture = observe(source);
  return fixture->nodes[fixture->wrong_parameter ? OUTPUT : INPUT].subject;
}

static perimortem_uuid operation(const void* source) {
  observe(source);
  const perimortem_uuid id = {0x89005d7d25b845b0ULL, 0xaca7f08286eb36e6ULL};
  return id;
}

static const ttx_representation* block_form(const void* source) {
  return observe(source)->payload;
}

static ttx_data_status commit(const void* source, ttx_block_surface surface) {
  model_fixture* fixture = observe(source);
  ++fixture->reads;
  memcpy(surface.data, &fixture->literal, sizeof(fixture->literal));
  return TTX_DATA_SUCCESS;
}

static ttx_binding_status bind_value(const void* source, perimortem_uuid id, ttx_storage output) {
  model_fixture* fixture = observe(source);
  ++fixture->bindings;
  if (!equal(id, TTX_BLOCK_ACCESS_ID_HIGH, TTX_BLOCK_ACCESS_ID_LOW)) { return TTX_BINDING_UNSUPPORTED; }
  static const ttx_block_access_operations operations = {block_form, commit};
  const ttx_block_access api = {source, &operations};
  return ttx_binding_provide(fixture->forms.block, &api, output);
}

static ttx_binding_status supports_value(const void* source, perimortem_uuid id) {
  observe(source);
  return equal(id, TTX_BLOCK_ACCESS_ID_HIGH, TTX_BLOCK_ACCESS_ID_LOW)
             ? TTX_BINDING_SATISFIED : TTX_BINDING_UNSUPPORTED;
}

static ttx_semantic_query data(const void* source) {
  observe(source);
  const ttx_semantic_query query = {source, bind_value, supports_value};
  return query;
}

static ttx_binding_status bind(const void* source, perimortem_uuid id, ttx_storage output) {
  model_fixture* fixture = observe(source);
  const model_node* node = source;
  ++fixture->bindings;
  if (equal(id, TTX_ABSTRACT_ID_HIGH, TTX_ABSTRACT_ID_LOW)) {
    return ttx_binding_provide(fixture->forms.abstract, &node->subject, output);
  }
  if (node->kind == FUNCTION && equal(id, TETRODOTOXIN_MODEL_EXECUTION_FUNCTION_ID_HIGH, TETRODOTOXIN_MODEL_EXECUTION_FUNCTION_ID_LOW)) {
    if (fixture->refusal) { return fixture->refusal; }
    const tetrodotoxin_model_execution_function api = {source, operation, parameters, results, body};
    return ttx_binding_provide(fixture->forms.function, &api, output);
  }
  if (node->kind == TYPE) {
    if (equal(id, TETRODOTOXIN_MODEL_TYPE_UNSIGNED_ID_HIGH, TETRODOTOXIN_MODEL_TYPE_UNSIGNED_ID_LOW)) {
      return ttx_binding_marker(output);
    }
    if (equal(id, TETRODOTOXIN_MODEL_TYPE_STORAGE_ID_HIGH, TETRODOTOXIN_MODEL_TYPE_STORAGE_ID_LOW)) {
      const tetrodotoxin_model_type_storage api = {source, storage};
      return ttx_binding_provide(fixture->forms.type_storage, &api, output);
    }
    if (equal(id, TETRODOTOXIN_MODEL_TYPE_CONVERSION_ID_HIGH, TETRODOTOXIN_MODEL_TYPE_CONVERSION_ID_LOW)) {
      const tetrodotoxin_model_type_conversion api = {source, convert};
      return ttx_binding_provide(fixture->forms.conversion, &api, output);
    }
  }
  if ((node->kind == CONSTANT || node->kind == PARAMETER) && equal(id, TTX_DOMAIN_ID_HIGH, TTX_DOMAIN_ID_LOW)) {
    static const ttx_domain_ops operations = {domain};
    const ttx_domain api = {source, &operations};
    return ttx_binding_provide(fixture->forms.domain, &api, output);
  }
  if ((node->kind == INPUT || node->kind == OUTPUT) && equal(id, TETRODOTOXIN_MODEL_EXECUTION_FIELD_ID_HIGH, TETRODOTOXIN_MODEL_EXECUTION_FIELD_ID_LOW)) {
    const tetrodotoxin_model_execution_field api = {source, type};
    return ttx_binding_provide(fixture->forms.field, &api, output);
  }
  if (node->kind == PARAMETER && equal(id, TETRODOTOXIN_MODEL_EXECUTION_PARAMETER_ID_HIGH, TETRODOTOXIN_MODEL_EXECUTION_PARAMETER_ID_LOW)) {
    const tetrodotoxin_model_execution_parameter api = {source, parameter};
    return ttx_binding_provide(fixture->forms.parameter, &api, output);
  }
  if (node->kind == CONSTANT && equal(id, TETRODOTOXIN_MODEL_EXECUTION_CONSTANT_ID_HIGH, TETRODOTOXIN_MODEL_EXECUTION_CONSTANT_ID_LOW)) {
    return ttx_binding_marker(output);
  }
  if (node->kind == CONSTANT && equal(id, TETRODOTOXIN_MODEL_EXECUTION_VALUE_ID_HIGH, TETRODOTOXIN_MODEL_EXECUTION_VALUE_ID_LOW)) {
    const tetrodotoxin_model_execution_value api = {source, data};
    return ttx_binding_provide(fixture->forms.value, &api, output);
  }
  if (node->kind == RETURN && equal(id, TETRODOTOXIN_MODEL_EXECUTION_RETURN_ID_HIGH, TETRODOTOXIN_MODEL_EXECUTION_RETURN_ID_LOW)) {
    const tetrodotoxin_model_execution_return api = {source, values};
    return ttx_binding_provide(fixture->forms.returned, &api, output);
  }
  return TTX_BINDING_UNSUPPORTED;
}

static perimortem_view_bytes bytes(const void* source) {
  observe(source);
  const perimortem_view_bytes result = {0, 0};
  return result;
}

static ttx_abstract resolve(const void* source) { observe(source); return ((const model_node*)source)->subject; }
static ttx_abstract lookup(const void* source, perimortem_view_bytes route) { (void)route; observe(source); return ttx_none(); }
static void visit(const void* source, ttx_concept_visitor visitor) { (void)visitor; observe(source); }



static ttx_binding_status supports(const void* source, perimortem_uuid id) {
  const model_fixture* fixture = observe(source);
  const model_node* node = source;
  if (equal(id, TTX_ABSTRACT_ID_HIGH, TTX_ABSTRACT_ID_LOW)) { return TTX_BINDING_SATISFIED; }
  if (node->kind == TYPE && (equal(id, TETRODOTOXIN_MODEL_TYPE_UNSIGNED_ID_HIGH, TETRODOTOXIN_MODEL_TYPE_UNSIGNED_ID_LOW) ||
      equal(id, TETRODOTOXIN_MODEL_TYPE_STORAGE_ID_HIGH, TETRODOTOXIN_MODEL_TYPE_STORAGE_ID_LOW) ||
      equal(id, TETRODOTOXIN_MODEL_TYPE_CONVERSION_ID_HIGH, TETRODOTOXIN_MODEL_TYPE_CONVERSION_ID_LOW))) { return TTX_BINDING_SATISFIED; }
  if (node->kind == FUNCTION && equal(id, TETRODOTOXIN_MODEL_EXECUTION_FUNCTION_ID_HIGH, TETRODOTOXIN_MODEL_EXECUTION_FUNCTION_ID_LOW)) { return fixture->refusal; }
  if ((node->kind == CONSTANT || node->kind == PARAMETER) && equal(id, TTX_DOMAIN_ID_HIGH, TTX_DOMAIN_ID_LOW)) { return TTX_BINDING_SATISFIED; }
  if (node->kind == CONSTANT && (equal(id, TETRODOTOXIN_MODEL_EXECUTION_CONSTANT_ID_HIGH, TETRODOTOXIN_MODEL_EXECUTION_CONSTANT_ID_LOW) ||
      equal(id, TETRODOTOXIN_MODEL_EXECUTION_VALUE_ID_HIGH, TETRODOTOXIN_MODEL_EXECUTION_VALUE_ID_LOW))) { return TTX_BINDING_SATISFIED; }
  if (node->kind == PARAMETER && equal(id, TETRODOTOXIN_MODEL_EXECUTION_PARAMETER_ID_HIGH, TETRODOTOXIN_MODEL_EXECUTION_PARAMETER_ID_LOW)) { return TTX_BINDING_SATISFIED; }
  if ((node->kind == INPUT || node->kind == OUTPUT) && equal(id, TETRODOTOXIN_MODEL_EXECUTION_FIELD_ID_HIGH, TETRODOTOXIN_MODEL_EXECUTION_FIELD_ID_LOW)) { return TTX_BINDING_SATISFIED; }
  if (node->kind == RETURN && equal(id, TETRODOTOXIN_MODEL_EXECUTION_RETURN_ID_HIGH, TETRODOTOXIN_MODEL_EXECUTION_RETURN_ID_LOW)) { return TTX_BINDING_SATISFIED; }
  return TTX_BINDING_UNSUPPORTED;
}

ttx_abstract model_fixture_open(model_fixture* fixture, model_forms forms, U8 mode) {
  static const ttx_abstract_ops operations = {supports, bind, bytes, resolve, lookup, visit};
  memset(fixture, 0, sizeof(*fixture));
  fixture->forms = forms;
  fixture->payload = forms.storage;
  fixture->mode = mode;
  fixture->alive = 1;
  fixture->literal = 12345;
  for (U8 i = 0; i < 7; ++i) {
    fixture->nodes[i].owner = fixture;
    fixture->nodes[i].kind = i;
    fixture->nodes[i].subject.source = &fixture->nodes[i];
    fixture->nodes[i].subject.operations = &operations;
  }
  return fixture->nodes[FUNCTION].subject;
}
