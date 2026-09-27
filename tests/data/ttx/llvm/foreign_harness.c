// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
#include <inttypes.h>
#include <stdint.h>

#include "Validation.Foreign/1.0/c_abi.h"

extern uint64_t library_foreign_state;

int run_foreign_integration(void) {
  const uint64_t first =
      TTX_FUNC_Validation_2eForeign__Foreign__library_5fnative_static();
  const uint64_t second =
      TTX_FUNC_Validation_2eForeign__Foreign__library_5fnative_static();
  return first == UINT64_C(22) && second == UINT64_C(42) &&
                 library_foreign_state == UINT64_C(40) &&
                 TTX_FUNC_Validation_2eForeign__Foreign__cross_5fartifact_static() ==
                     UINT64_C(15)
             ? 0
             : 1;
}
