// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_SOURCE_CONTRACTS_DOCUMENTATION_H
#define TETRODOTOXIN_SOURCE_CONTRACTS_DOCUMENTATION_H

#include "perimortem/core/view/bytes.h"
#include "perimortem/core/perimortem.h"

// Documentation may borrow authored text, generate prose, or combine lines
// from several owners. Requiring a flat buffer would make those providers
// assemble another representation just so a consumer could read it. Asking
// for a count and individual lines preserves their order while letting each
// provider keep the representation it already has.
//
// Borrowing those lines avoids copying them into a second metadata store.
// The consumer therefore keeps the supplying owner and its operation code
// alive while reading. Each returned line remains readable for the lifetime
// promised for this view, rather than ending when get_line returns.
//
// An owner without prose supplies zero lines so missing documentation does
// not become a failed semantic query. It still supplies both operations, and
// an index beyond the line count returns an empty byte view. Consumers can
// therefore use the same interface for authored and empty documentation.
typedef struct tetrodotoxin_source_documentation {
  const void* source;
  const struct tetrodotoxin_source_documentation_ops* operations;
} tetrodotoxin_source_documentation;

typedef struct tetrodotoxin_source_documentation_ops {
  Count (*line_count)(const void* source);
  perimortem_view_bytes (*get_line)(const void* source, Count index);
} tetrodotoxin_source_documentation_ops;

#endif
