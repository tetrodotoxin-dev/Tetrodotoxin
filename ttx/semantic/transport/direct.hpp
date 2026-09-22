// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "ttx/data/protocol/direct.hpp"
#include "ttx/semantic/transport/direct.h"

namespace Ttx::Semantic::Transport {

// These UUIDs let independently loaded readers and providers agree on
// Direct's C operation tables without sharing C++ type identities. The payload
// descriptors still establish which representation the returned pointer exposes.
// Binding grants only the requested Direct role. Other transports require
// their own agreement even when native code could read the published bytes.
class Direct {
 public:
  // Binding View declares that the reader accepts a directly published ABI.
  // Its descriptor remains the requirement even when no target Storage exists yet.
  struct View : Data::Protocol::Direct::View {
    using Data::Protocol::Direct::View::View;
    static constexpr Perimortem::System::Uuid contract_id{
      TTX_DIRECT_VIEW_ID_HIGH,
      TTX_DIRECT_VIEW_ID_LOW,
    };

    using Api = ttx_direct_view;
    using Operations = ttx_direct_view_operations;

    static auto accept(Api api) -> Bool {
      return api.operations && api.operations->representation;
    }
  };

  // Binding Access grants the public pointer under its publication lifetime.
  // Agreement on this identity does not grant any other transport capability.
  struct Access : Data::Protocol::Direct::Access {
    using Data::Protocol::Direct::Access::Access;
    static constexpr Perimortem::System::Uuid contract_id{
      TTX_DIRECT_ACCESS_ID_HIGH,
      TTX_DIRECT_ACCESS_ID_LOW,
    };

    using Api = ttx_direct_access;
    using Operations = ttx_direct_access_operations;

    static auto accept(Api api) -> Bool {
      return api.operations && api.operations->representation && api.operations->read_ptr;
    }
  };
};
}  // namespace Ttx::Semantic::Transport
