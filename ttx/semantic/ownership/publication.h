// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_SEMANTIC_OWNERSHIP_PUBLICATION_H
#define TTX_SEMANTIC_OWNERSHIP_PUBLICATION_H

#include "ttx/semantic/negotiation/query.h"

// A publication transfers one provider lifetime with its Query. Release
// receives `query.source` and is called exactly once after every borrowed view
// has ended. The provider may release a graph, a factory or a runtime instance
// through the same boundary. The receiver does not need to know its
// representation.
//
// Executable code has a separate lifetime. A compiler can release its discovery
// publication while keeping the module and independently emitted factories.
// Release must run before the module containing its implementation is unloaded.
typedef struct ttx_publication {
  ttx_semantic_query query;
  void (*release)(const void* source);
} ttx_publication;

#endif
