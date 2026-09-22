// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"

namespace Perimortem::Abi::Core {

inline constexpr Perimortem::Core::View::Bytes object_allocate_symbol =
    "perimortem_core_object_allocate"_view;
inline constexpr Perimortem::Core::View::Bytes object_allocate_buffer_symbol =
    "perimortem_core_object_allocate_buffer"_view;
inline constexpr Perimortem::Core::View::Bytes object_retain_symbol =
    "perimortem_core_object_retain"_view;
inline constexpr Perimortem::Core::View::Bytes object_release_symbol =
    "perimortem_core_object_release"_view;
inline constexpr Perimortem::Core::View::Bytes object_capacity_symbol =
    "perimortem_core_object_capacity"_view;
inline constexpr Perimortem::Core::View::Bytes object_clone_symbol =
    "perimortem_core_object_clone"_view;
inline constexpr Perimortem::Core::View::Bytes object_reservations_symbol =
    "perimortem_core_object_reservations"_view;
inline constexpr Perimortem::Core::View::Bytes object_reserve_symbol =
    "perimortem_core_object_reserve"_view;
inline constexpr Perimortem::Core::View::Bytes object_finalize_trivial_symbol =
    "perimortem_core_object_finalize_trivial"_view;
}  // namespace Perimortem::Abi::Core

extern "C" auto perimortem_core_object_allocate(
    const Perimortem::Core::Object<>::Descriptor* descriptor) -> U8*;
extern "C" auto perimortem_core_object_allocate_buffer(
    const Perimortem::Core::Object<>::Descriptor* descriptor,
    Count count,
    Count element_size) -> U8*;
extern "C" auto perimortem_core_object_retain(U8* payload) -> void;
extern "C" auto perimortem_core_object_release(U8* payload) -> void;
extern "C" auto perimortem_core_object_capacity(U8* payload) -> Count;
extern "C" auto perimortem_core_object_clone(
    U8* payload,
    const Perimortem::Core::Object<>::Descriptor* descriptor,
    Count element_size) -> U8*;
extern "C" auto perimortem_core_object_reservations(U8* payload) -> Count;
extern "C" auto perimortem_core_object_reserve(
    U8* payload,
    const Perimortem::Core::Object<>::Descriptor* descriptor,
    Count count,
    Count element_size,
    const U8* default_value) -> U8*;
extern "C" auto perimortem_core_object_finalize_trivial(U8*) -> void;
