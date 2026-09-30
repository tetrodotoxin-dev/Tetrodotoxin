// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "consumer.h"

#include <string.h>

#include "tetrodotoxin/dialects/source/stream.h"
#include "tetrodotoxin/dialects/source/token.h"
#include "ttx/concept/capabilities/borrow.h"
#include "ttx/concept/capabilities/import.h"

// The observer uses only the C operation tables to acquire retained access.
// This keeps the ABI check independent of the C++ Stream and Borrow adapters.
static void observe(void* state, ttx_abstract graph) {
  source_consumer* consumer = state;
  ttx_borrow borrow;
  perimortem_uuid id;
  ttx_storage target;
  id.high = TTX_BORROW_ID_HIGH;
  id.low = TTX_BORROW_ID_LOW;
  target.representation = ttx_borrow_representation();
  target.data = (U8*)&borrow;
  target.size = sizeof(borrow);

  if (graph.operations->bind(graph.source, id, target) !=
      TTX_BINDING_SATISFIED) {
    consumer->failed = 1;
    return;
  }

  consumer->failed |=
      borrow.operations->borrow(borrow.source, &consumer->retained) !=
      TTX_BINDING_SATISFIED;
}

// The host supplies its own compiled input representation and mutable bytes.
// Source must accept that independent description through its released Import
// contract rather than relying on a provider local representation address.
ttx_binding_status source_consumer_open(
    source_consumer* consumer,
    ttx_semantic_query query,
    const tetrodotoxin_source_input* input) {
  ttx_import importer;
  perimortem_uuid id;
  ttx_storage target;
  id.high = TTX_IMPORT_ID_HIGH;
  id.low = TTX_IMPORT_ID_LOW;
  target.representation = ttx_import_representation();
  target.data = (U8*)&importer;
  target.size = sizeof(importer);

  if (query.bind(query.source, id, target) != TTX_BINDING_SATISFIED) {
    return TTX_BINDING_REJECTED;
  }

  const ttx_binding_status status = importer.operations->visit(
      importer.source, input, consumer->input, consumer, observe);
  return consumer->failed ? TTX_BINDING_REJECTED : status;
}

// The host overwrites its path and text after the import callback ends.
// Reading their original contents through the C ABI proves that Source kept
// its own bytes alive. The same retained answer is released before unloading.
int source_consumer_finish(source_consumer* consumer) {
  if (!consumer->retained.source) {
    return 1;
  }

  tetrodotoxin_source_stream stream;
  perimortem_uuid id;
  ttx_storage target;
  const ttx_borrowed retained = consumer->retained;
  int failed = consumer->failed;
  id.high = TETRODOTOXIN_SOURCE_STREAM_ID_HIGH;
  id.low = TETRODOTOXIN_SOURCE_STREAM_ID_LOW;
  target.representation = consumer->stream;
  target.data = (U8*)&stream;
  target.size = sizeof(stream);

  if (retained.operations->abstract.bind(retained.source, id, target) !=
      TTX_BINDING_SATISFIED) {
    failed = 1;
  } else {
    const tetrodotoxin_source_snapshot input =
        stream.operations->input(stream.source);
    const tetrodotoxin_source_tokens tokens =
        stream.operations->tokens(stream.source);
    tetrodotoxin_source_token token = {0};
    if (!tokens.representation ||
        !ttx_representation_compatible(
            consumer->token, tokens.representation) ||
        !tokens.data || tokens.count != 4 || tokens.size < 4 * sizeof(token)) {
      failed = 1;
    } else {
      memcpy(&token, tokens.data + 2 * sizeof(token), sizeof(token));
    }
    const perimortem_uuid lexicon = stream.operations->lexicon(stream.source);
    failed |= lexicon.high != TETRODOTOXIN_SOURCE_LEXICON_ID_HIGH ||
              lexicon.low != TETRODOTOXIN_SOURCE_LEXICON_ID_LOW;
    failed |= input.text.size != 18 ||
              memcmp(input.text.data, "alpha 42\r\n// note\n", 18) != 0;
    failed |= input.path.size != 12 ||
              memcmp(input.path.data, "consumer.ttx", 12) != 0;
    failed |= token.code != 1 || token.offset != 10 || token.line != 2 ||
              token.column != 1;
  }

  retained.operations->release(retained.source);
  consumer->retained.source = NULL;
  consumer->retained.operations = NULL;
  return failed;
}
