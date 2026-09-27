// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
#include <stdint.h>

const uint64_t library_foreign_bias = UINT64_C(2);
uint64_t library_foreign_state = UINT64_C(0);

uint64_t library_foreign_add(uint64_t left, uint64_t right) {
  return left + right;
}
