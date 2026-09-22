// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_SYSTEM_UUID_H
#define PERIMORTEM_SYSTEM_UUID_H

#include "perimortem/core/perimortem.h"

// UUID provides enough bits that collisions are extremely unlikely. The ABI
// uses Perimortem's dual U64 format as the wire format since it's compact and
// already has support by the C++ runtime.
//
// High holds the first sixteen hexadecimal digits of the canonical spelling.
// Persistent formats should maintain the little endian high/low structure or
// serialize the UUID in its string form (with or without `-`).
typedef struct perimortem_uuid {
  U64 high;
  U64 low;
} perimortem_uuid;

#endif
