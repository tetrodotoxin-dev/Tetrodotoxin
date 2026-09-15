// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/data/form/compiler.hpp"

#include "ttx/data/form/representation.hpp"

using namespace Ttx::Data;
using namespace Ttx::Data::Form;

// The C entry prepares the description before asking its owner to allocate
// the exact output size. The view and bytes share that owner's lifetime, while
// Compiler destruction releases the temporary preparation storage.
auto ttx_representation_compile(
    ttx_schema_reference schema,
    ttx_representation_allocator allocator,
    const ttx_representation** result) -> ttx_data_status {
  if (!schema.is_set() || !allocator.allocate || !result) {
    return TTX_DATA_INVALID;
  }

  Compiler compiler;
  const auto status = compiler.compile(schema);
  if (status != Status::Success) {
    return static_cast<ttx_data_status>(status);
  }

  const Count size = compiler.get_size();
  if (size > Count(-1) - sizeof(Representation)) {
    return TTX_DATA_OVERFLOW;
  }

  auto* allocation = allocator.allocate(
      allocator.source, sizeof(Representation) + size, alignof(Representation));
  if (!allocation) {
    return TTX_DATA_BOUNDS;
  }

  auto* bytes = static_cast<U8*>(allocation) + sizeof(Representation);
  const auto written =
      compiler.write(Perimortem::Core::Access::Bytes(bytes, size));
  if (written != Status::Success) {
    return static_cast<ttx_data_status>(written);
  }

  *result = new (allocation, Perimortem::Core::Placement::Construct)
      Representation(bytes, size);
  return TTX_DATA_SUCCESS;
}
