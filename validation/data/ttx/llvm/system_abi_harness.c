// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
#include <stddef.h>

#include "Validation.SystemAbi/1.0/c_abi.h"

_Static_assert(
    sizeof(ttx_validation_systemabi_SystemAbi_Dynamic_Bytes) == 16,
    "Dynamic::Bytes carrier changed");
_Static_assert(
    offsetof(ttx_validation_systemabi_SystemAbi_Dynamic_Bytes, data) == 0,
    "Bytes data offset changed");
_Static_assert(
    offsetof(ttx_validation_systemabi_SystemAbi_Dynamic_Bytes, size) == 8,
    "Bytes size offset changed");
_Static_assert(
    sizeof(ttx_validation_systemabi_SystemAbi_Option_5bBytes_5d) == 24,
    "Option[Dynamic::Bytes] carrier changed");
_Static_assert(
    offsetof(ttx_validation_systemabi_SystemAbi_Option_5bBytes_5d, set) == 16,
    "Option[Dynamic::Bytes] state offset changed");

int main(void) {
  return TTX_FUNC_Validation_2eSystemAbi__SystemAbi__system_5froundtrip_static()
             ? 0
             : 1;
}
