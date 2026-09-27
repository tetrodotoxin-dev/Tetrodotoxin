// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_SOURCE_TOKENIZATION_H
#define TETRODOTOXIN_SOURCE_TOKENIZATION_H

#include "ttx/concept/abstract.h"
#include "ttx/semantic/ownership/publication.h"

#define TETRODOTOXIN_SOURCE_TOKENIZATION_ID_HIGH 0xdd3c5840c6024a24ULL
#define TETRODOTOXIN_SOURCE_TOKENIZATION_ID_LOW 0xaf84e472e2b6298aULL

// Source bytes do not choose their lexical meaning. The host selects a
// Tokenization policy and gives it an immutable observation. That policy reads
// Content through the observation's own binding surface and publishes its
// classified input for whichever dialect understands that token contract.
// Consequently this interface neither fixes a token format nor exports a
// native Cursor or allocation service.
//
// Satisfied transfers one output Publication. Every other status leaves output
// untouched and transfers no ownership. The provider cleans up unsuccessful
// work. Lexical errors may be represented in a successful token publication
// according to its own contract. Pending is an unsettled answer, not a promise
// to write output after this synchronous call has returned.
//
// The caller retains the input observation, injected provider services and
// executable code until the output is released. The provider owns its output
// allocation and may borrow those inputs without copying their semantic graph.
typedef struct tetrodotoxin_source_tokenization {
  const void* source;
  ttx_binding_status (*tokenize)(
      const void* source, ttx_abstract observation, ttx_publication* output);
} tetrodotoxin_source_tokenization;

PERIMORTEM_C const ttx_representation*
tetrodotoxin_source_tokenization_representation(void);

#endif
