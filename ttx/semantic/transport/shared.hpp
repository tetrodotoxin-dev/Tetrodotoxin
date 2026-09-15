// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "ttx/data/protocol/shared.hpp"
#include "ttx/semantic/transport/shared.h"

namespace Ttx::Semantic::Transport {

// Shared's UUIDs let a reader accept an acquired lifetime and a provider supply
// it synchronously. This contract gives Flow a release obligation to retain
// alongside the payload pointer, keeping that payload valid across operations.
class Shared {
 public:
  // Binding View permits Flow to hold an acquired representation for the
  // reader. The descriptor describes what that held representation must satisfy.
  struct View : Data::Protocol::Shared::View {
    using Data::Protocol::Shared::View::View;
    static constexpr Perimortem::System::Uuid contract_id{
      TTX_SHARED_VIEW_ID_HIGH,
      TTX_SHARED_VIEW_ID_LOW,
    };

    using Api = ttx_shared_view;
    using Operations = ttx_shared_view_operations;

    static auto accept(Api api) -> Bool {
      return api.operations && api.operations->representation;
    }
  };

  // Binding Access supplies acquisition and its release obligation. Flow can
  // retain the resulting lifetime across operations instead of acquiring again.
  struct Access : Data::Protocol::Shared::Access {
    using Data::Protocol::Shared::Access::Access;
    static constexpr Perimortem::System::Uuid contract_id{
      TTX_SHARED_ACCESS_ID_HIGH,
      TTX_SHARED_ACCESS_ID_LOW,
    };

    using Api = ttx_shared_access;
    using Operations = ttx_shared_access_operations;

    static auto accept(Api api) -> Bool {
      return api.operations && api.operations->representation && api.operations->acquire;
    }
  };
};
}  // namespace Ttx::Semantic::Transport
