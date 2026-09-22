// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "ttx/data/protocol/block.hpp"
#include "ttx/semantic/transport/block.h"

namespace Ttx::Semantic::Transport {

// Block negotiation asks whether a reader can supply a complete surface and a
// provider can populate it. The UUIDs name those two Data roles independently
// of their implementations. A caller needing a provider pointer or individual
// field reads must negotiate the corresponding separate protocol.
class Block {
 public:
  // Binding View supplies the reader's way to expose a target surface. The
  // surface is requested per call so independently owned results stay separate.
  struct View : Data::Protocol::Block::View {
    using Data::Protocol::Block::View::View;
    static constexpr Perimortem::System::Uuid contract_id{
      TTX_BLOCK_VIEW_ID_HIGH,
      TTX_BLOCK_VIEW_ID_LOW,
    };

    using Api = ttx_block_view;
    using Operations = ttx_block_view_operations;

    static auto accept(Api api) -> Bool {
      return api.operations && api.operations->representation && api.operations->surface;
    }
  };

  // Binding Access allows the provider to fill a complete surface on request.
  // Its returned status reports the result without lending its own data
  // pointer.
  struct Access : Data::Protocol::Block::Access {
    using Data::Protocol::Block::Access::Access;
    static constexpr Perimortem::System::Uuid contract_id{
      TTX_BLOCK_ACCESS_ID_HIGH,
      TTX_BLOCK_ACCESS_ID_LOW,
    };

    using Api = ttx_block_access;
    using Operations = ttx_block_access_operations;

    static auto accept(Api api) -> Bool {
      return api.operations && api.operations->representation && api.operations->commit;
    }
  };
};
}  // namespace Ttx::Semantic::Transport
