// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
#include <stddef.h>

#include "Perimortem.System/1.0/c_abi.h"
#include "Perimortem.Memory/1.0/c_abi.h"

_Static_assert(
    sizeof(ttx_perimortem_memory_Dynamic_Bytes) == 16,
    "Package qualified Bytes carrier changed");
_Static_assert(
    offsetof(ttx_perimortem_memory_Dynamic_Bytes, data) == 0,
    "Provider header must own imported carrier fields");
_Static_assert(
    offsetof(ttx_perimortem_memory_Dynamic_Bytes, size) == 8,
    "Provider header must own imported carrier layout");
