// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "ttx/data/protocol/fragment.hpp"
#include "ttx/semantic/transport/fragment.h"

namespace Ttx::Semantic::Transport {

// Fragment negotiation permits a representation to be assembled from separate
// observations. These UUIDs connect a reader accepting that access model with
// the provider's typed getters. Each getter can produce its value when called,
// so a snapshot requires an additional policy. The bound source stays opaque
// and grants no access to provider storage.
class Fragment {
 public:
  // Binding View accepts separate typed observations in the required representation.
  // Any stronger consistency between observations belongs to the reader policy.
  struct View : Data::Protocol::Fragment::View {
    using Data::Protocol::Fragment::View::View;
    static constexpr Perimortem::System::Uuid contract_id{
      TTX_FRAGMENT_VIEW_ID_HIGH,
      TTX_FRAGMENT_VIEW_ID_LOW,
    };

    using Api = ttx_fragment_view;
    using Operations = ttx_fragment_view_operations;

    static auto accept(Api api) -> Bool {
      return api.operations && api.operations->representation;
    }
  };

  // Binding Access supplies only the typed getters that its representation describes.
  // A representation assembled by observing them need not exist in the
  // provider.
  struct Access : Data::Protocol::Fragment::Access {
    using Data::Protocol::Fragment::Access::Access;
    static constexpr Perimortem::System::Uuid contract_id{
      TTX_FRAGMENT_ACCESS_ID_HIGH,
      TTX_FRAGMENT_ACCESS_ID_LOW,
    };

    using Api = ttx_fragment_access;
    using Operations = ttx_fragment_access_operations;

    static auto accept(Api api) -> Bool {
      return api.operations && api.operations->representation;
    }
  };
};
}  // namespace Ttx::Semantic::Transport
