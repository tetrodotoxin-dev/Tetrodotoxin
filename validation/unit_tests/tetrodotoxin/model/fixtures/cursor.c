// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/tetrodotoxin/model/fixtures/cursor.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define REPORT_CAPACITY 16

typedef struct cursor_state {
  ttx_abstract origin;
  U8* text;
  cursor_fixture_entry* entries;
  Count count;
  Count observations;
  tetrodotoxin_source_diagnostic reports[REPORT_CAPACITY];
  Count report_count;
  U32* releases;
} cursor_state;

static tetrodotoxin_source_token token_at(const cursor_state* self, Count index) {
  const tetrodotoxin_source_token result = {
    ((self->count - index + 17) << 8) | self->entries[index].code};
  return result;
}

static Count index_of(const cursor_state* self, tetrodotoxin_source_token token) {
  return self->count + 17 - (token.value >> 8);
}

static tetrodotoxin_source_token get_token(const void* source, U64 index) {
  cursor_state* self = (cursor_state*)source;
  ++self->observations;
  if (index >= self->count) { index = self->count - 1; }
  return token_at(self, index);
}

static perimortem_view_bytes get_text(const void* source, tetrodotoxin_source_token token) {
  const cursor_state* self = source;
  const tetrodotoxin_source_range extent = self->entries[index_of(self, token)].extent;
  const perimortem_view_bytes result = {self->text + extent.offset, extent.size};
  return result;
}

static void get_anchor(const void* source, tetrodotoxin_source_span span,
                       const tetrodotoxin_source_token* focus, tetrodotoxin_source_anchor* output) {
  const cursor_state* self = source;
  const tetrodotoxin_source_range first = self->entries[index_of(self, span.start)].extent;
  const tetrodotoxin_source_range last = self->entries[index_of(self, span.end)].extent;
  const Count start = first.offset < last.offset ? first.offset : last.offset;
  const Count first_end = first.offset + first.size;
  const Count last_end = last.offset + last.size;
  const Count end = first_end > last_end ? first_end : last_end;
  *output = (tetrodotoxin_source_anchor){self->origin, {start, end - start}, {0, 0}, 0};
  if (focus) {
    output->focus = self->entries[index_of(self, *focus)].extent;
    output->has_focus = 1;
  }
}

static U64 get_error_count(const void* source) {
  return ((const cursor_state*)source)->report_count;
}

static perimortem_view_bytes copy_text(perimortem_view_bytes text) {
  U8* data = malloc(text.size ? text.size : 1);
  assert(data);
  if (text.size) { memcpy(data, text.data, text.size); }
  const perimortem_view_bytes result = {data, text.size};
  return result;
}

static void report(const void* source, const tetrodotoxin_source_anchor* anchor,
                   perimortem_view_bytes message, perimortem_view_bytes hint) {
  cursor_state* self = (cursor_state*)source;
  assert(self->report_count < REPORT_CAPACITY);
  tetrodotoxin_source_diagnostic* error = &self->reports[self->report_count++];
  memset(error, 0, sizeof(*error));
  if (anchor) { error->anchor = *anchor; error->has_anchor = 1; }
  error->message = copy_text(message);
  error->hint = copy_text(hint);
}

static U8 get_error(const void* source, U64 index, tetrodotoxin_source_diagnostic* output) {
  const cursor_state* self = source;
  if (index >= self->report_count) { return 0; }
  *output = self->reports[index];
  return 1;
}

static ttx_binding_status supports(const void* source, perimortem_uuid id) {
  (void)source;
  return id.high == TETRODOTOXIN_SOURCE_LEXICAL_CURSOR_ID_HIGH &&
                 id.low == TETRODOTOXIN_SOURCE_LEXICAL_CURSOR_ID_LOW
             ? TTX_BINDING_SATISFIED : TTX_BINDING_UNSUPPORTED;
}

static ttx_binding_status bind(const void* source, perimortem_uuid id, ttx_storage output) {
  const ttx_binding_status status = supports(source, id);
  if (status != TTX_BINDING_SATISFIED) { return status; }
  static const tetrodotoxin_source_cursor_ops operations = {
    get_token, get_text, get_anchor, get_error_count, report, get_error};
  const tetrodotoxin_source_cursor api = {source, &operations, 0};
  return ttx_binding_provide(tetrodotoxin_source_cursor_representation(), &api, output);
}

static void release(const void* source) {
  cursor_state* self = (cursor_state*)source;
  ++*self->releases;
  for (Count i = 0; i < self->report_count; ++i) {
    free((void*)self->reports[i].message.data);
    free((void*)self->reports[i].hint.data);
  }
  free(self->entries);
  free(self->text);
  free(self);
}

ttx_publication cursor_fixture_open(ttx_abstract origin, perimortem_view_bytes text,
    const cursor_fixture_entry* entries, Count count, U32* releases) {
  assert(count);
  cursor_state* self = calloc(1, sizeof(*self));
  assert(self);
  self->origin = origin;
  self->text = (U8*)copy_text(text).data;
  self->entries = malloc(sizeof(*entries) * count);
  assert(self->entries);
  memcpy(self->entries, entries, sizeof(*entries) * count);
  self->count = count;
  self->releases = releases;
  const ttx_publication output = {{self, bind, supports}, release};
  return output;
}

ttx_binding_status cursor_fixture_interpret(ttx_semantic_query dialect,
    tetrodotoxin_source_cursor* cursor, ttx_abstract context, ttx_publication* output) {
  tetrodotoxin_source_dialect api = {0};
  const perimortem_uuid id = {TETRODOTOXIN_SOURCE_DIALECT_ID_HIGH, TETRODOTOXIN_SOURCE_DIALECT_ID_LOW};
  const ttx_storage target = {tetrodotoxin_source_dialect_representation(), (U8*)&api, sizeof(api)};
  const ttx_binding_status status = dialect.bind(dialect.source, id, target);
  if (status != TTX_BINDING_SATISFIED) { return status; }
  return api.interpret(api.source, cursor, context, output);
}

U64 cursor_fixture_observations(ttx_semantic_query query) {
  return ((const cursor_state*)query.source)->observations;
}
