// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "source_protocol.h"

#include <string.h>

static int same(perimortem_uuid id, U64 high, U64 low) {
  return id.high == high && id.low == low;
}

static ttx_binding_status supports(const void* source, perimortem_uuid id) {
  const source_protocol* state = source;
  if (same(id, TTX_ABSTRACT_ID_HIGH, TTX_ABSTRACT_ID_LOW) ||
      same(
          id, TETRODOTOXIN_SOURCE_PARSE_ID_HIGH,
          TETRODOTOXIN_SOURCE_PARSE_ID_LOW)) {
    return TTX_BINDING_SATISFIED;
  }
  if (same(
          id, TETRODOTOXIN_SOURCE_STREAM_ID_HIGH,
          TETRODOTOXIN_SOURCE_STREAM_ID_LOW)) {
    return state->refuse_stream ? TTX_BINDING_REJECTED : TTX_BINDING_SATISFIED;
  }
  if (same(
          id, TETRODOTOXIN_SOURCE_DIAGNOSTICS_ID_HIGH,
          TETRODOTOXIN_SOURCE_DIAGNOSTICS_ID_LOW)) {
    return state->refuse_diagnostics ? TTX_BINDING_REJECTED
                                     : TTX_BINDING_SATISFIED;
  }
  return TTX_BINDING_UNKNOWN;
}

static ttx_binding_status
    bind(const void* source, perimortem_uuid id, ttx_storage target) {
  source_protocol* state = (source_protocol*)source;
  const ttx_binding_status status = supports(source, id);
  if (status != TTX_BINDING_SATISFIED) {
    return status;
  }
  if (same(
          id, TETRODOTOXIN_SOURCE_STREAM_ID_HIGH,
          TETRODOTOXIN_SOURCE_STREAM_ID_LOW)) {
    const tetrodotoxin_source_stream stream = source_protocol_stream(state);
    return ttx_binding_provide(state->stream_representation, &stream, target);
  }
  if (same(
          id, TETRODOTOXIN_SOURCE_PARSE_ID_HIGH,
          TETRODOTOXIN_SOURCE_PARSE_ID_LOW)) {
    const tetrodotoxin_source_parse parser = source_protocol_parse(state);
    return ttx_binding_provide(state->parse_representation, &parser, target);
  }
  if (same(
          id, TETRODOTOXIN_SOURCE_DIAGNOSTICS_ID_HIGH,
          TETRODOTOXIN_SOURCE_DIAGNOSTICS_ID_LOW)) {
    const tetrodotoxin_source_diagnostics sink =
        source_protocol_diagnostics(state);
    return ttx_binding_provide(
        state->diagnostics_representation, &sink, target);
  }
  const tetrodotoxin_source_stream stream = source_protocol_stream(state);
  const ttx_abstract abstract = {source, &stream.operations->abstract};
  return ttx_binding_provide(ttx_abstract_representation(), &abstract, target);
}

static perimortem_view_bytes data(const void* source) {
  return ((const source_protocol*)source)->input.text;
}
static ttx_abstract resolve(const void* source) {
  const tetrodotoxin_source_stream stream =
      source_protocol_stream((source_protocol*)source);
  const ttx_abstract result = {source, &stream.operations->abstract};
  return result;
}
static ttx_abstract lookup(const void* source, perimortem_view_bytes route) {
  (void)route;
  return resolve(source);
}
static void visit(const void* source, ttx_concept_visitor receiver) {
  (void)source;
  (void)receiver;
}
static tetrodotoxin_source_snapshot input(const void* source) {
  return ((const source_protocol*)source)->input;
}
static tetrodotoxin_source_tokens tokens(const void* source) {
  return ((const source_protocol*)source)->tokens;
}
static perimortem_uuid lexicon(const void* source) {
  return ((const source_protocol*)source)->lexicon;
}

static void report(
    const void* source,
    tetrodotoxin_source_snapshot input,
    tetrodotoxin_source_anchor anchor,
    perimortem_view_bytes message,
    perimortem_view_bytes hint) {
  source_protocol* state = (source_protocol*)source;
  (void)anchor;
  (void)hint;
  ++state->reports;
  memset(state->message, 0, sizeof(state->message));
  memset(state->evidence, 0, sizeof(state->evidence));
  memcpy(state->message, message.data, message.size < 63 ? message.size : 63);
  memcpy(
      state->evidence, input.text.data,
      input.text.size < 63 ? input.text.size : 63);
}
static Count count(const void* source) {
  return ((const source_protocol*)source)->reports;
}

// This fixture admits one Addressable and returns the original source subject
// as its projection. It exercises admission, effects and retained source access
// without claiming a production language or manufacturing an AST contract.
static ttx_binding_status parse(
    const void* source,
    tetrodotoxin_source_cursor* cursor,
    ttx_abstract diagnostics,
    void* receiver,
    void (*receive)(void*, ttx_abstract)) {
  const source_protocol* state = source;
  if (!cursor || !receive || !cursor->stream.source ||
      !cursor->stream.operations || !diagnostics.source ||
      !diagnostics.operations) {
    return TTX_BINDING_REJECTED;
  }
  const tetrodotoxin_source_stream stream = cursor->stream;
  const perimortem_uuid vocabulary = stream.operations->lexicon(stream.source);
  if (!same(
          vocabulary, TETRODOTOXIN_SOURCE_LEXICON_ID_HIGH,
          TETRODOTOXIN_SOURCE_LEXICON_ID_LOW)) {
    return TTX_BINDING_REJECTED;
  }
  const tetrodotoxin_source_tokens buffer =
      stream.operations->tokens(stream.source);
  if (!buffer.representation || !buffer.data || !buffer.count ||
      !ttx_representation_compatible(
          state->token_representation, buffer.representation) ||
      buffer.count > buffer.size / sizeof(tetrodotoxin_source_token) ||
      cursor->begin > cursor->index || cursor->index > cursor->end ||
      cursor->end >= buffer.count) {
    return TTX_BINDING_REJECTED;
  }
  tetrodotoxin_source_diagnostics sink;
  const perimortem_uuid id = {
    TETRODOTOXIN_SOURCE_DIAGNOSTICS_ID_HIGH,
    TETRODOTOXIN_SOURCE_DIAGNOSTICS_ID_LOW};
  const ttx_storage target = {
    state->diagnostics_representation, (U8*)&sink, sizeof(sink)};
  const ttx_binding_status status =
      diagnostics.operations->bind(diagnostics.source, id, target);
  if (status != TTX_BINDING_SATISFIED) {
    return status;
  }
  if (cursor->index == cursor->end) {
    return TTX_BINDING_REJECTED;
  }
  tetrodotoxin_source_token token;
  memcpy(&token, buffer.data + cursor->index * sizeof(token), sizeof(token));
  ++cursor->index;
  if (token.code != 4) {
    const tetrodotoxin_source_anchor anchor = {token, token, token};
    const perimortem_view_bytes message = {(const U8*)"Expected a name.", 16};
    const perimortem_view_bytes hint = {NULL, 0};
    sink.operations->report(
        sink.source, stream.operations->input(stream.source), anchor, message,
        hint);
    return TTX_BINDING_REJECTED;
  }
  const ttx_abstract result = {stream.source, &stream.operations->abstract};
  receive(receiver, result);
  return TTX_BINDING_SATISFIED;
}

tetrodotoxin_source_stream source_protocol_stream(source_protocol* state) {
  static const tetrodotoxin_source_stream_ops operations = {
    {supports, bind, data, resolve, lookup, visit}, input, tokens, lexicon};
  const tetrodotoxin_source_stream result = {state, &operations};
  return result;
}
tetrodotoxin_source_parse source_protocol_parse(source_protocol* state) {
  static const tetrodotoxin_source_parse_ops operations = {
    {supports, bind, data, resolve, lookup, visit}, parse};
  const tetrodotoxin_source_parse result = {state, &operations};
  return result;
}
tetrodotoxin_source_diagnostics source_protocol_diagnostics(
    source_protocol* state) {
  static const tetrodotoxin_source_diagnostics_ops operations = {
    {supports, bind, data, resolve, lookup, visit}, report, count};
  const tetrodotoxin_source_diagnostics result = {state, &operations};
  return result;
}
