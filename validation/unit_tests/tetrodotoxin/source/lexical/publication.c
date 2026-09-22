// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/tetrodotoxin/source/lexical/publication.h"

ttx_binding_status source_read_cursor(
    ttx_semantic_query query, tetrodotoxin_source_cursor* output) {
  const perimortem_uuid id = {
    TETRODOTOXIN_SOURCE_LEXICAL_CURSOR_ID_HIGH,
    TETRODOTOXIN_SOURCE_LEXICAL_CURSOR_ID_LOW};
  const ttx_storage storage = {
    tetrodotoxin_source_cursor_representation(),
    (U8*)output, sizeof(*output)};
  return query.bind(query.source, id, storage);
}

U64 source_consume_cursor(tetrodotoxin_source_cursor* cursor) {
  const tetrodotoxin_source_token token =
      cursor->operations->get_token(cursor->source, cursor->index);
  if ((U8)token.value != TETRODOTOXIN_TOKEN_TERMINAL) {
    ++cursor->index;
  }
  return token.value;
}

static ttx_binding_status reject_after_consuming(
    const void* source, tetrodotoxin_source_cursor* cursor,
    ttx_abstract context, ttx_publication* output) {
  (void)source;
  (void)context;
  (void)output;
  source_consume_cursor(cursor);
  return TTX_BINDING_REJECTED;
}

tetrodotoxin_source_dialect source_rejecting_dialect(void) {
  const tetrodotoxin_source_dialect api = {0, reject_after_consuming};
  return api;
}
