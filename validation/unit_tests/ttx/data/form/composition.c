// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/ttx/data/form/composition.h"

// This fixture knows no C++ owner. It composes the same admitted child through
// the C entry and asks the caller for publication storage only after admission.
ttx_data_status compose_pair(
    const ttx_representation* child, Count width, Count alignment,
    ttx_representation_allocator allocator, const ttx_representation** result) {
  const ttx_representation_member members[] = {{child, 0}, {child, width}};
  return ttx_representation_compose(members, 2, width * 2, alignment, allocator, result);
}
