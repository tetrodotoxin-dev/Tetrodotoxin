// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/tetrodotoxin/source/declaration.h"

// The C provider publishes its actual record. Null state represents a valid
// synthetic declaration with no provenance, not a failed binding operation.
static U8 get_anchor(const void* source, tetrodotoxin_source_anchor* output) {
  if (!source) {
    return 0;
  }

  *output = *(const tetrodotoxin_source_anchor*)source;
  return 1;
}

static ttx_binding_status bind(
    const void* source, perimortem_uuid id, ttx_storage destination) {
  if (id.high != TETRODOTOXIN_SOURCE_DECLARATION_ID_HIGH ||
      id.low != TETRODOTOXIN_SOURCE_DECLARATION_ID_LOW) {
    return TTX_BINDING_UNSUPPORTED;
  }

  const tetrodotoxin_source_declaration api = {source, get_anchor};
  return ttx_binding_provide(
      tetrodotoxin_source_declaration_representation(), &api, destination);
}

static ttx_binding_status supports(const void* source, perimortem_uuid id) {
  (void)source;
  return id.high == TETRODOTOXIN_SOURCE_DECLARATION_ID_HIGH &&
                 id.low == TETRODOTOXIN_SOURCE_DECLARATION_ID_LOW
             ? TTX_BINDING_SATISFIED : TTX_BINDING_UNSUPPORTED;
}

ttx_semantic_query source_declaration_fixture(
    const tetrodotoxin_source_anchor* anchor) {
  const ttx_semantic_query query = {anchor, bind, supports};
  return query;
}
