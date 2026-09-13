// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "perimortem/utility/result.hpp"

#include "ttx/data/form/representation.hpp"
#include "ttx/semantic/binding.hpp"
#include "ttx/semantic/bound.hpp"
#include "ttx/semantic/thunk.h"

namespace Ttx::Semantic {

// Thunk is the negotiated bridge from a semantic promise to callable code.
// Its Handle checks the realization once, allowing the resulting contract view
// to invoke ordinary typed function pointers without repeating negotiation.
class Thunk {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_THUNK_ID_HIGH,
    TTX_THUNK_ID_LOW,
  };

  enum class Convention : U32 { SystemVAMD64 = TTX_CALLING_SYSTEM_V_AMD64 };

  using Operations = ttx_thunk_operations;

  // The bootstrap Query and every fulfilled answer borrow their publication.
  // Keeping that lifetime outside Handle lets an image, source generation or
  // loaded module retain many interfaces without one allocation per binding.
  class Handle : public Bound<Operations> {
   public:
    using Bound::Bound;

    auto fulfill(
        Perimortem::System::Uuid contract,
        Convention convention,
        const Data::Form::Representation& representation) const
        -> Perimortem::Utility::Result<Binding, Binding::Failure>;
  };
};

}  // namespace Ttx::Semantic
