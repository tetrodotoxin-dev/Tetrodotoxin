// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/tetrodotoxin/source/content.h"

#include <stdlib.h>

#include "ttx/concept/answers/none.h"
#include "ttx/semantic/transport/block.h"

typedef struct observation {
  const ttx_representation* form;
  Count size;
  U8 first;
  ttx_binding_status acceptance;
  U8 fail_read;
  U32* releases;
  U32 reads;
} observation;

static ttx_binding_status supports(const void* source, perimortem_uuid id) {
  const observation* self = source;
  if (id.high == TETRODOTOXIN_SOURCE_CONTENT_ID_HIGH &&
      id.low == TETRODOTOXIN_SOURCE_CONTENT_ID_LOW) {
    return self->acceptance;
  }

  if ((id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW) ||
      (id.high == TTX_BLOCK_ACCESS_ID_HIGH && id.low == TTX_BLOCK_ACCESS_ID_LOW)) {
    return TTX_BINDING_SATISFIED;
  }

  return TTX_BINDING_UNSUPPORTED;
}

static U64 size(const void* source) {
  return ((const observation*)source)->size;
}

static const ttx_representation* representation(const void* source) {
  return ((const observation*)source)->form;
}

static ttx_data_status commit(const void* source, ttx_block_surface surface) {
  const observation* self = source;
  ++((observation*)source)->reads;
  if (self->fail_read) {
    return TTX_DATA_IO_ERROR;
  }

  for (Count i = 0; i < surface.size; ++i) {
    surface.data[i] = self->first + i % 26;
  }

  return TTX_DATA_SUCCESS;
}

static perimortem_view_bytes data(const void* source) {
  (void)source;
  const perimortem_view_bytes empty = {0};
  return empty;
}

static ttx_abstract missing(const void* source, perimortem_view_bytes route) {
  (void)source;
  (void)route;
  return ttx_none();
}

static void visit(const void* source, ttx_concept_visitor visitor) {
  (void)source;
  (void)visitor;
}

// These declarations only tie the self-referential Abstract and Query tables
// together. Their bind path still checks the actual C API representation.
static ttx_binding_status bind(const void*, perimortem_uuid, ttx_storage);
static ttx_abstract resolve(const void*);
static const ttx_abstract_ops operations = {
  supports, bind, data, resolve, missing, visit};

static ttx_abstract resolve(const void* source) {
  const ttx_abstract result = {source, &operations};
  return result;
}

static ttx_semantic_query bytes(const void* source) {
  const ttx_semantic_query result = {source, bind, supports};
  return result;
}

static ttx_binding_status bind(
    const void* source, perimortem_uuid id, ttx_storage output) {
  const ttx_binding_status status = supports(source, id);
  if (status != TTX_BINDING_SATISFIED) {
    return status;
  }

  if (id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW) {
    const ttx_abstract api = resolve(source);
    return ttx_binding_provide(ttx_abstract_representation(), &api, output);
  }

  if (id.high == TETRODOTOXIN_SOURCE_CONTENT_ID_HIGH &&
      id.low == TETRODOTOXIN_SOURCE_CONTENT_ID_LOW) {
    const tetrodotoxin_source_content api = {source, size, bytes};
    return ttx_binding_provide(
        tetrodotoxin_source_content_representation(), &api, output);
  }

  static const ttx_block_access_operations block = {representation, commit};
  const ttx_block_access api = {source, &block};
  return ttx_binding_provide(ttx_block_access_representation(), &api, output);
}

static void release(const void* source) {
  const observation* self = source;
  ++*self->releases;
  free((void*)self);
}

ttx_publication source_content_open(
    const ttx_representation* form, Count size, U8 first,
    ttx_binding_status acceptance, U8 fail_read, U32* releases) {
  observation* self = malloc(sizeof(*self));
  if (!self) {
    abort();
  }

  *self = (observation){form, size, first, acceptance, fail_read, releases, 0};
  const ttx_publication result = {bytes(self), release};
  return result;
}

U32 source_content_reads(ttx_semantic_query query) {
  return ((const observation*)query.source)->reads;
}

static ttx_binding_status tokenization_supports(
    const void* source, perimortem_uuid id) {
  (void)source;
  return id.high == TETRODOTOXIN_SOURCE_TOKENIZATION_ID_HIGH &&
                 id.low == TETRODOTOXIN_SOURCE_TOKENIZATION_ID_LOW
             ? TTX_BINDING_SATISFIED : TTX_BINDING_UNSUPPORTED;
}

static ttx_binding_status tokenization_bind(
    const void* source, perimortem_uuid id, ttx_storage output) {
  const ttx_binding_status status = tokenization_supports(source, id);
  if (status != TTX_BINDING_SATISFIED) {
    return status;
  }

  return ttx_binding_provide(
      tetrodotoxin_source_tokenization_representation(), source, output);
}

ttx_semantic_query source_tokenization_query(
    const tetrodotoxin_source_tokenization* service) {
  const ttx_semantic_query query = {
    service, tokenization_bind, tokenization_supports};
  return query;
}
