// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/input.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;

auto Input::set(KeyBits& keys, Key key) -> void {
  Count index = Count(key);
  if (key == Key::None || index >= key_count) {
    return;
  }
  keys[index / word_bits] |= U64(1) << (index % word_bits);
}

auto Input::contains(const KeyBits& keys, Key key) -> Bool {
  Count index = Count(key);
  if (key == Key::None || index >= key_count) {
    return False;
  }
  return Bool((keys[index / word_bits] & (U64(1) << (index % word_bits))) != 0);
}

auto Input::add_modifier_groups(KeyBits& keys) -> void {
  if (contains(keys, Key::LeftShift) || contains(keys, Key::RightShift)) {
    set(keys, Key::Shift);
  }
  if (contains(keys, Key::LeftControl) || contains(keys, Key::RightControl)) {
    set(keys, Key::Control);
  }
  if (contains(keys, Key::LeftAlt) || contains(keys, Key::RightAlt)) {
    set(keys, Key::Alt);
  }
  if (contains(keys, Key::LeftSuper) || contains(keys, Key::RightSuper)) {
    set(keys, Key::Super);
  }
}

auto Input::collect(View::Vector<Key> keys, const Mapping& mapping) -> KeyBits {
  KeyBits output;
  for (Key physical : keys) {
    set(output, mapping.resolve(physical));
  }
  add_modifier_groups(output);
  return output;
}

auto Input::create(View::Vector<Key> current, View::Vector<Key> previous)
    -> Input {
  return create(current, previous, Mapping(), Pointer());
}

auto Input::create(
    View::Vector<Key> current,
    View::Vector<Key> previous,
    Pointer pointer) -> Input {
  return create(current, previous, Mapping(), pointer);
}

auto Input::create(
    View::Vector<Key> current,
    View::Vector<Key> previous,
    const Mapping& mapping) -> Input {
  return create(current, previous, mapping, Pointer());
}

auto Input::create(
    View::Vector<Key> current_keys,
    View::Vector<Key> previous_keys,
    const Mapping& mapping,
    Pointer pointer) -> Input {
  KeyBits current = collect(current_keys, mapping);
  KeyBits previous = collect(previous_keys, mapping);
  KeyBits changed;
  for (Count index = 0; index < word_count; index++) {
    changed[index] = current[index] ^ previous[index];
  }
  return Input(current, changed, pointer);
}

auto Input::is_current(Key key) const -> Bool {
  return contains(current, key);
}

auto Input::is_pressed(Key key) const -> Bool {
  return contains(current, key) && contains(changed, key);
}

auto Input::is_released(Key key) const -> Bool {
  return !contains(current, key) && contains(changed, key);
}
