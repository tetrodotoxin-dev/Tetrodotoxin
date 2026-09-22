// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_CORE_VIEW_BYTES_H
#define PERIMORTEM_CORE_VIEW_BYTES_H

#include "perimortem/core/perimortem.h"

// Foreign data may already live in storage that Perimortem does't own so the
// ABI uses Perimortem's View::Bytes format of a pointer and byte count.
typedef struct perimortem_view_bytes {
  const U8* data;
  Count size;
} perimortem_view_bytes;

#endif
