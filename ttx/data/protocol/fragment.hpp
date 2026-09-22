// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/data/form/representation.hpp"
#include "ttx/data/protocol/fragment.h"

namespace Ttx::Data::Protocol {

// Fragment supplies individual observations without lending a backing record.
// The C++ interface returns a value or failure from each synchronous call,
// keeping the C output parameter inside the implementation boundary.
class Fragment {
 public:
  // View accepts the primitive observations described by its representation. It
  // needs no destination storage to establish that requirement with a provider.
  class View {
   public:
    using Operations = ttx_fragment_view_operations;

    constexpr View(const void* source, const Operations& operations)
        : value{source, &operations} {}

    constexpr explicit View(ttx_fragment_view value) : value(value) {}

    auto get_representation() const -> const Form::Representation& {
      return *value.operations->representation(value.source);
    }

    constexpr auto get_abi() const -> ttx_fragment_view { return value; }

   private:
    ttx_fragment_view value;
  };

  // Access finishes each observation before returning. A provider may compute
  // the value during that call, but it cannot retain an output for later work.
  class Access {
   public:
    using Operations = ttx_fragment_access_operations;

    constexpr Access(const void* source, const Operations& operations)
        : value{source, &operations} {}

    constexpr explicit Access(ttx_fragment_access value) : value(value) {}

    auto get_representation() const -> const Form::Representation& {
      return *value.operations->representation(value.source);
    }

    constexpr auto get_abi() const -> ttx_fragment_access { return value; }

    auto get_u8(Count position) const
        -> Perimortem::Utility::Result<U8, Status>;

    auto get_u16(Count position) const
        -> Perimortem::Utility::Result<U16, Status>;

    auto get_u32(Count position) const
        -> Perimortem::Utility::Result<U32, Status>;

    auto get_u64(Count position) const
        -> Perimortem::Utility::Result<U64, Status>;

    auto get_s8(Count position) const
        -> Perimortem::Utility::Result<S8, Status>;

    auto get_s16(Count position) const
        -> Perimortem::Utility::Result<S16, Status>;

    auto get_s32(Count position) const
        -> Perimortem::Utility::Result<S32, Status>;

    auto get_s64(Count position) const
        -> Perimortem::Utility::Result<S64, Status>;

    auto get_r32(Count position) const
        -> Perimortem::Utility::Result<R32, Status>;

    auto get_r64(Count position) const
        -> Perimortem::Utility::Result<R64, Status>;

    auto get_pointer(Count position) const
        -> Perimortem::Utility::Result<void*, Status>;

    auto get_v64(Count position) const
        -> Perimortem::Utility::Result<Form::Schema::V64, Status>;
    auto get_v128(Count position) const
        -> Perimortem::Utility::Result<Form::Schema::V128, Status>;
    auto get_v256(Count position) const
        -> Perimortem::Utility::Result<Form::Schema::V256, Status>;
    auto get_v512(Count position) const
        -> Perimortem::Utility::Result<Form::Schema::V512, Status>;

   private:
    ttx_fragment_access value;
  };
};

}  // namespace Ttx::Data::Protocol

// These are the C Fragment output carriers, not native SIMD register types.
// Their byte arrays must remain structs when they occur in a callable ABI.
TTX_DATA_RECORD(ttx_vector64, TTX_DATA_MEMBER(ttx_vector64, bytes));
TTX_DATA_RECORD(ttx_vector128, TTX_DATA_MEMBER(ttx_vector128, bytes));
TTX_DATA_RECORD(ttx_vector256, TTX_DATA_MEMBER(ttx_vector256, bytes));
TTX_DATA_RECORD(ttx_vector512, TTX_DATA_MEMBER(ttx_vector512, bytes));

TTX_DATA_RECORD(
    ttx_fragment_view_operations,
    TTX_DATA_MEMBER(ttx_fragment_view_operations, representation));

TTX_DATA_RECORD(
    ttx_fragment_access_operations,
    TTX_DATA_MEMBER(ttx_fragment_access_operations, representation),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_u8),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_u16),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_u32),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_u64),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_s8),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_s16),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_s32),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_s64),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_r32),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_r64),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_pointer),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_v64),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_v128),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_v256),
    TTX_DATA_MEMBER(ttx_fragment_access_operations, get_v512));

TTX_DATA_RECORD(
    ttx_fragment_view,
    TTX_DATA_MEMBER(ttx_fragment_view, source),
    TTX_DATA_MEMBER(ttx_fragment_view, operations));

TTX_DATA_RECORD(
    ttx_fragment_access,
    TTX_DATA_MEMBER(ttx_fragment_access, source),
    TTX_DATA_MEMBER(ttx_fragment_access, operations));
