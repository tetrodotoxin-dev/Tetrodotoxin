// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/tetrodotoxin/dialect/fixtures/lifetime.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "tetrodotoxin/source/declaration.h"
#include "tetrodotoxin/source/dialect.h"
#include "ttx/concept/answers/none.h"
#include "ttx/concept/modules/module.h"

static monograph_lifetime* lifetime;
static const perimortem_uuid abstract_id = {TTX_ABSTRACT_ID_HIGH, TTX_ABSTRACT_ID_LOW};
static const perimortem_uuid dialect_id = {
  TETRODOTOXIN_SOURCE_DIALECT_ID_HIGH, TETRODOTOXIN_SOURCE_DIALECT_ID_LOW};
static const perimortem_uuid declaration_id = {
  TETRODOTOXIN_SOURCE_DECLARATION_ID_HIGH, TETRODOTOXIN_SOURCE_DECLARATION_ID_LOW};

static int same(perimortem_uuid a, perimortem_uuid b) {
  return a.high == b.high && a.low == b.low;
}

static int named(perimortem_view_bytes route, const char* name) {
  const size_t size = strlen(name);
  return route.size == size && memcmp(route.data, name, size) == 0;
}

static perimortem_view_bytes text(const char* value) {
  const perimortem_view_bytes result = {(const U8*)value, strlen(value)};
  return result;
}

// Each pair is a Monograph for one invocation, with its own source policy.
// Child storage is retained through Publications rather than merged into the
// parent's allocation. Those children may themselves be pairs from this module
// or Library results supplied by another module.
typedef struct pair_monograph {
  ttx_publication children[2];
  ttx_abstract subjects[2];
  Count size;
  tetrodotoxin_source_anchor anchor;
} pair_monograph;

static const ttx_abstract_ops pair_operations;

static ttx_binding_status pair_supports(const void* source, perimortem_uuid id) {
  (void)source;
  return same(id, abstract_id) || same(id, declaration_id)
             ? TTX_BINDING_SATISFIED : TTX_BINDING_UNSUPPORTED;
}

static U8 anchor(const void* source, tetrodotoxin_source_anchor* output) {
  *output = ((const pair_monograph*)source)->anchor;
  return 1;
}

static ttx_binding_status pair_bind(const void* source, perimortem_uuid id, ttx_storage target) {
  if (same(id, abstract_id)) {
    const ttx_abstract value = {source, &pair_operations};
    return ttx_binding_provide(ttx_abstract_representation(), &value, target);
  }

  if (same(id, declaration_id)) {
    const tetrodotoxin_source_declaration value = {source, anchor};
    return ttx_binding_provide(tetrodotoxin_source_declaration_representation(), &value, target);
  }

  return TTX_BINDING_UNSUPPORTED;
}

static perimortem_view_bytes pair_data(const void* source) {
  (void)source;
  return text("pair");
}

static ttx_abstract pair_resolve(const void* source) {
  const ttx_abstract value = {source, &pair_operations};
  return value;
}

static ttx_abstract pair_lookup(const void* source, perimortem_view_bytes route) {
  const pair_monograph* pair = source;
  if (named(route, "left")) { return pair->subjects[0]; }
  if (named(route, "right")) { return pair->subjects[1]; }
  return ttx_none();
}

static void pair_visit(const void* source, ttx_concept_visitor visitor) {
  const pair_monograph* pair = source;
  visitor.receive(visitor.source, text("left"), pair->subjects[0]);
  visitor.receive(visitor.source, text("right"), pair->subjects[1]);
}

static const ttx_abstract_ops pair_operations = {
  pair_supports, pair_bind, pair_data, pair_resolve, pair_lookup, pair_visit};

static void release_pair(const void* source) {
  pair_monograph* pair = (pair_monograph*)source;
  // Unwind children before their enclosing invocation. The same path cleans
  // up an unpublished partial pair if the second child declines interpretation.
  while (pair->size) {
    ttx_publication child = pair->children[--pair->size];
    if (child.release) { child.release(child.query.source); }
    ++lifetime->children_released;
  }
  --lifetime->live;
  ++lifetime->releases;
  free(pair);
}

typedef struct dialect_provider {
  tetrodotoxin_source_dialect child;
  U8 mode;
} dialect_provider;

enum { PAIR, FORWARD, REJECT, PENDING, UNSUPPORTED, MODE_COUNT };
static const ttx_abstract_ops provider_operations;

static ttx_binding_status interpret(
    const void* source, tetrodotoxin_source_cursor* cursor, ttx_abstract context,
    ttx_publication* output) {
  const dialect_provider* provider = source;
  if (provider->mode == FORWARD) {
    // Forwarding transfers the child's existing lifetime, without creating
    // another owner or changing the Abstract policy that the child published.
    return provider->child.interpret(provider->child.source, cursor, context, output);
  }

  pair_monograph* pair = calloc(1, sizeof(*pair));
  assert(pair);
  ++lifetime->live;
  const U64 start = cursor->index;
  const tetrodotoxin_source_token first =
      cursor->operations->get_token(cursor->source, cursor->index);
  ttx_binding_status status = TTX_BINDING_SATISFIED;
  for (Count i = 0; i < 2; ++i) {
    status = provider->child.interpret(provider->child.source, cursor, context, &pair->children[i]);
    if (status != TTX_BINDING_SATISFIED) { break; }
    ++pair->size;

    const ttx_storage target = {
      ttx_abstract_representation(), (U8*)&pair->subjects[i], sizeof(pair->subjects[i])};
    status = pair->children[i].query.bind(pair->children[i].query.source, abstract_id, target);
    if (status != TTX_BINDING_SATISFIED) { break; }

    if (provider->mode != PAIR) {
      status = provider->mode == REJECT ? TTX_BINDING_REJECTED :
               provider->mode == PENDING ? TTX_BINDING_PENDING : TTX_BINDING_UNSUPPORTED;
      break;
    }
  }

  if (status != TTX_BINDING_SATISFIED) {
    release_pair(pair);
    return status;
  }

  const tetrodotoxin_source_token last =
      cursor->operations->get_token(cursor->source, cursor->index > start ? cursor->index - 1 : start);
  const tetrodotoxin_source_span span = {first, last};
  cursor->operations->get_anchor(cursor->source, span, 0, &pair->anchor);
  *output = (ttx_publication){{pair, pair_bind, pair_supports}, release_pair};
  return TTX_BINDING_SATISFIED;
}

static ttx_binding_status provider_supports(const void* source, perimortem_uuid id) {
  (void)source;
  return same(id, abstract_id) || same(id, dialect_id)
             ? TTX_BINDING_SATISFIED : TTX_BINDING_UNSUPPORTED;
}

static ttx_binding_status provider_bind(const void* source, perimortem_uuid id, ttx_storage target) {
  if (same(id, abstract_id)) {
    const ttx_abstract value = {source, &provider_operations};
    return ttx_binding_provide(ttx_abstract_representation(), &value, target);
  }

  if (same(id, dialect_id)) {
    const tetrodotoxin_source_dialect value = {source, interpret};
    return ttx_binding_provide(tetrodotoxin_source_dialect_representation(), &value, target);
  }

  return TTX_BINDING_UNSUPPORTED;
}

static perimortem_view_bytes provider_data(const void* source) {
  (void)source;
  return text("composition fixture");
}

static ttx_abstract provider_resolve(const void* source) {
  const ttx_abstract value = {source, &provider_operations};
  return value;
}

static const char* modes[] = {"pair", "forward", "reject", "pending", "unsupported"};

static ttx_abstract provider_lookup(const void* source, perimortem_view_bytes route) {
  const dialect_provider* provider = source;
  const dialect_provider* root = provider - provider->mode;
  for (U8 i = 0; i < MODE_COUNT; ++i) {
    if (named(route, modes[i])) { return provider_resolve(root + i); }
  }
  return ttx_none();
}

static void provider_visit(const void* source, ttx_concept_visitor visitor) {
  const dialect_provider* provider = source;
  const dialect_provider* root = provider - provider->mode;
  for (U8 i = 0; i < MODE_COUNT; ++i) {
    visitor.receive(visitor.source, text(modes[i]), provider_resolve(root + i));
  }
}

static const ttx_abstract_ops provider_operations = {
  provider_supports, provider_bind, provider_data, provider_resolve, provider_lookup, provider_visit};

static void release_provider(const void* source) {
  ++lifetime->acquisition_releases;
  free((void*)source);
}

void monograph_fixture_watch(monograph_lifetime* value) { lifetime = value; }

// Module acquisition receives the host-selected child dialect. The fixture
// binds its complete API once, retaining no assumption about the child's
// language, allocation scheme or result structure.
ttx_data_status ttx_module_open(ttx_semantic_query host, ttx_module_acquisition* output) {
  if (!host.bind) { return TTX_DATA_UNSUPPORTED; }
  tetrodotoxin_source_dialect child = {0};
  const ttx_storage target = {tetrodotoxin_source_dialect_representation(), (U8*)&child, sizeof(child)};
  if (host.bind(host.source, dialect_id, target) != TTX_BINDING_SATISFIED) {
    return TTX_DATA_INCOMPATIBLE;
  }

  dialect_provider* providers = calloc(MODE_COUNT, sizeof(*providers));
  assert(providers);
  for (U8 i = 0; i < MODE_COUNT; ++i) {
    providers[i].child = child;
    providers[i].mode = i;
  }
  ++lifetime->acquisitions;
  *output = (ttx_module_acquisition){provider_resolve(providers), providers, release_provider};
  return TTX_DATA_SUCCESS;
}

__attribute__((destructor)) static void unloaded(void) {
  if (lifetime) {
    assert(lifetime->live == 0);
    assert(lifetime->acquisitions == lifetime->acquisition_releases);
    ++lifetime->unloaded;
  }
}
