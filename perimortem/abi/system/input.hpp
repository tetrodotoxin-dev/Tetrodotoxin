// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/system/input.hpp"

#include "perimortem/math/vec2d.hpp"

namespace Perimortem::Abi::System {

// Input is the trivial carrier generated TTX code sees at the native boundary.
// It mirrors one immutable Perimortem snapshot without exposing the C++ class
// or the platform events that produced it.
struct Input {
  Core::Static::Vector<U64, 3> current;
  Core::Static::Vector<U64, 3> changed;
  Math::Vec2D pointer;
  Math::Vec2D pointer_delta;
  Math::Vec2D scroll;
  bool pointer_active;
};

static_assert(sizeof(Input) == sizeof(Perimortem::System::Input));
static_assert(alignof(Input) == alignof(Perimortem::System::Input));
static_assert(__is_trivially_copyable(Input));
static_assert(__is_standard_layout(Input));

// The application runtime publishes once after collecting a frame. TTX reads
// that value through the C boundary, so every query during the frame observes
// the same snapshot even when native events continue arriving.
auto create_input(const Perimortem::System::Input& input) -> Input;
auto is_current(const Input& input, U8 key) -> Bool;
auto is_changed(const Input& input, U8 key) -> Bool;
auto publish_input(const Perimortem::System::Input& input) -> void;

}  // namespace Perimortem::Abi::System

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"

extern "C" auto perimortem_system_input_snapshot()
    -> Perimortem::Abi::System::Input;
extern "C" auto perimortem_system_input_held(
    Perimortem::Abi::System::Input input,
    U8 key) -> bool;
extern "C" auto perimortem_system_input_pressed(
    Perimortem::Abi::System::Input input,
    U8 key) -> bool;
extern "C" auto perimortem_system_input_released(
    Perimortem::Abi::System::Input input,
    U8 key) -> bool;

#pragma clang diagnostic pop
