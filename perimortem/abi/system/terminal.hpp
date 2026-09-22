// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/abi/core/option.hpp"
#include "perimortem/abi/memory/dynamic/bytes.hpp"

namespace Perimortem::Abi::System::Terminal {

inline constexpr Perimortem::Core::View::Bytes read_line_symbol =
    "perimortem_system_terminal_read_line"_view;
inline constexpr Perimortem::Core::View::Bytes write_line_symbol =
    "perimortem_system_terminal_write_line"_view;

static_assert(
    sizeof(Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>) ==
    sizeof(Perimortem::Abi::Core::Option<
           Perimortem::Abi::Memory::Dynamic::Bytes>));
static_assert(__is_trivial(
    Perimortem::Abi::Core::Option<Perimortem::Abi::Memory::Dynamic::Bytes>));
static_assert(__is_standard_layout(
    Perimortem::Abi::Core::Option<Perimortem::Abi::Memory::Dynamic::Bytes>));
static_assert(
    alignof(Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>) ==
    alignof(Count));
static_assert(
    alignof(Perimortem::Abi::Core::Option<
            Perimortem::Abi::Memory::Dynamic::Bytes>) == alignof(Count));
static_assert(
    Perimortem::Abi::Core::Option<
        Perimortem::Abi::Memory::Dynamic::Bytes>::get_value_offset() == 0);
static_assert(
    Perimortem::Abi::Core::Option<
        Perimortem::Abi::Memory::Dynamic::Bytes>::get_state_offset() ==
    sizeof(Perimortem::Abi::Memory::Dynamic::Bytes));

}  // namespace Perimortem::Abi::System::Terminal

// These functions expose the ordinary Perimortem Terminal through the exact
// value carriers used by TTX. Returned Bytes ownership transfers to the caller,
// while the write argument remains borrowed for the duration of the call.
extern "C" auto perimortem_system_terminal_read_line()
    -> Perimortem::Abi::Core::Option<Perimortem::Abi::Memory::Dynamic::Bytes>;

extern "C" auto perimortem_system_terminal_write_line(
    Perimortem::Abi::Memory::Dynamic::Bytes data) -> bool;
