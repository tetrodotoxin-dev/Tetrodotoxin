// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/abi/system/input.hpp"

using namespace Perimortem;

static thread_local Abi::System::Input current_input;

auto Abi::System::create_input(const Perimortem::System::Input& input)
    -> Input {
  Abi::System::Input output = {};
  for (Count index = 0; index < 3; index++) {
    output.current[index] = input.get_current_word(index);
    output.changed[index] = input.get_changed_word(index);
  }
  const Perimortem::System::Input::Pointer& pointer = input.get_pointer();
  output.pointer = {pointer.x, pointer.y};
  output.pointer_delta = {pointer.delta_x, pointer.delta_y};
  output.scroll = {pointer.scroll_x, pointer.scroll_y};
  output.pointer_active = bool(pointer.active);
  return output;
}

auto Abi::System::is_current(const Input& input, U8 key) -> Bool {
  return key != 0 && key < U8(Perimortem::System::Input::Key::Count) &&
         Bool((input.current[key / 64] & (U64(1) << (key % 64))) != 0);
}

auto Abi::System::is_changed(const Input& input, U8 key) -> Bool {
  return key != 0 && key < U8(Perimortem::System::Input::Key::Count) &&
         Bool((input.changed[key / 64] & (U64(1) << (key % 64))) != 0);
}

auto Abi::System::publish_input(const Perimortem::System::Input& input)
    -> void {
  current_input = create_input(input);
}

extern "C" auto perimortem_system_input_snapshot() -> Abi::System::Input {
  return current_input;
}

extern "C" auto perimortem_system_input_held(Abi::System::Input input, U8 key)
    -> bool {
  return bool(Abi::System::is_current(input, key));
}

extern "C" auto perimortem_system_input_pressed(
    Abi::System::Input input,
    U8 key) -> bool {
  return bool(
      Abi::System::is_current(input, key) &&
      Abi::System::is_changed(input, key));
}

extern "C" auto perimortem_system_input_released(
    Abi::System::Input input,
    U8 key) -> bool {
  return bool(
      !Abi::System::is_current(input, key) &&
      Abi::System::is_changed(input, key));
}
