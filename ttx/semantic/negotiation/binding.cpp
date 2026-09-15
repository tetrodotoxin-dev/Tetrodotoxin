// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/semantic/negotiation/binding.hpp"

using namespace Ttx::Semantic::Negotiation;

auto ttx_binding_provide(
    const ttx_representation* representation, const void* api,
    ttx_storage requested) -> ttx_binding_status {
  if (!representation->compatible(*requested.representation)) {
    return TTX_BINDING_REJECTED;
  }

  const Count size = representation->get_extent();
  if (size) {
    memmove(requested.data, api, size);
  }

  return TTX_BINDING_SATISFIED;
}

auto ttx_binding_marker(ttx_storage requested) -> ttx_binding_status {
  return requested.representation->get_extent() == 0 ? TTX_BINDING_SATISFIED
                                                     : TTX_BINDING_REJECTED;
}
