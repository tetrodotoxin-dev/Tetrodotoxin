// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"

namespace Perimortem::System {

// Input is one immutable frame snapshot over a host neutral key space. The
// selected platform finishes collection and remapping before publishing this
// value, so application code sees stable actions instead of native event
// objects or keycodes.
class Input {
 public:
  // Key names physical controls rather than text produced by a keyboard
  // layout. Text entry can therefore use a layout aware service later while
  // gameplay and tools keep stable controls that are straightforward to remap.
  enum class Key : U8 {
    None,

    Escape,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,
    PrintScreen,
    ScrollLock,
    Pause,

    Grave,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,
    Digit0,
    Minus,
    Equal,
    Backspace,
    Tab,
    Q,
    W,
    E,
    R,
    T,
    Y,
    U,
    I,
    O,
    P,
    LeftBracket,
    RightBracket,
    Backslash,
    CapsLock,
    A,
    S,
    D,
    F,
    G,
    H,
    J,
    K,
    L,
    Semicolon,
    Apostrophe,
    Enter,
    LeftShift,
    Z,
    X,
    C,
    V,
    B,
    N,
    M,
    Comma,
    Period,
    Slash,
    RightShift,
    LeftControl,
    LeftSuper,
    LeftAlt,
    Space,
    RightAlt,
    RightSuper,
    Menu,
    RightControl,

    Insert,
    Delete,
    Home,
    End,
    PageUp,
    PageDown,
    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,

    NumLock,
    NumpadDivide,
    NumpadMultiply,
    NumpadSubtract,
    NumpadAdd,
    NumpadEnter,
    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
    NumpadDecimal,
    NumpadEqual,
    NumpadComma,

    NonUsBackslash,
    InternationalRo,
    InternationalYen,
    Kana,
    Convert,
    NonConvert,

    Mute,
    VolumeDown,
    VolumeUp,
    MediaPlayPause,
    MediaStop,
    MediaPrevious,
    MediaNext,
    BrowserBack,
    BrowserForward,
    BrowserRefresh,
    BrowserHome,

    MousePrimary,
    MouseSecondary,
    MouseMiddle,
    MouseBack,
    MouseForward,
    MouseSix,
    MouseSeven,
    MouseEight,

    // Aggregate modifiers let callers ask the common question while retaining
    // the left and right controls needed by detailed bindings.
    Shift,
    Control,
    Alt,
    Super,

    Count,
  };

  static constexpr Count key_count = Count(Key::Count);

  // Mapping translates each physical Key once into its final virtual Key.
  // Several controls may share one destination, while None disables a source.
  // A single step keeps bindings deterministic and leaves action composition
  // with the application domain that gives those actions meaning.
  class Mapping {
   public:
    constexpr Mapping() { reset(); }

    constexpr auto remap(Key source, Key destination) -> Bool {
      if (!is_source(source) || !is_key(destination)) {
        return False;
      }
      destinations[Count(source)] = destination;
      return True;
    }

    constexpr auto unmap(Key source) -> Bool {
      return remap(source, Key::None);
    }

    constexpr auto reset(Key source) -> Bool {
      if (!is_source(source)) {
        return False;
      }
      destinations[Count(source)] = source;
      return True;
    }

    constexpr auto reset() -> void {
      for (Count index = 0; index < key_count; index++) {
        destinations[index] = Key(index);
      }
    }

    constexpr auto resolve(Key source) const -> Key {
      return is_source(source) ? destinations[Count(source)] : Key::None;
    }

   private:
    static constexpr auto is_key(Key key) -> Bool {
      return Count(key) < key_count;
    }

    static constexpr auto is_source(Key key) -> Bool {
      return key != Key::None && is_key(key);
    }

    Core::Static::Vector<Key, key_count> destinations;
  };

  // Pointer uses surface local coordinates. Motion and scroll are accumulated
  // between snapshots, giving each frame one stable observation regardless of
  // how many native events produced it.
  struct Pointer {
    R32 x = 0;
    R32 y = 0;
    R32 delta_x = 0;
    R32 delta_y = 0;
    R32 scroll_x = 0;
    R32 scroll_y = 0;
    Bool active = False;
  };

  constexpr Input() = default;

  static auto create(
      Core::View::Vector<Key> current,
      Core::View::Vector<Key> previous) -> Input;

  static auto create(
      Core::View::Vector<Key> current,
      Core::View::Vector<Key> previous,
      Pointer pointer) -> Input;

  static auto create(
      Core::View::Vector<Key> current,
      Core::View::Vector<Key> previous,
      const Mapping& mapping) -> Input;

  static auto create(
      Core::View::Vector<Key> current,
      Core::View::Vector<Key> previous,
      const Mapping& mapping,
      Pointer pointer) -> Input;

  auto is_current(Key key) const -> Bool;
  auto is_pressed(Key key) const -> Bool;
  auto is_released(Key key) const -> Bool;

  constexpr auto get_current_word(Count index) const -> U64 {
    return index < word_count ? current[index] : U64(0);
  }

  constexpr auto get_changed_word(Count index) const -> U64 {
    return index < word_count ? changed[index] : U64(0);
  }

  constexpr auto get_pointer() const -> const Pointer& { return pointer; }

 private:
  static constexpr Count word_bits = sizeof(U64) * 8;
  static constexpr Count word_count = (key_count + word_bits - 1) / word_bits;
  using KeyBits = Core::Static::Vector<U64, word_count>;

  constexpr Input(KeyBits current, KeyBits changed, Pointer pointer)
      : current(current), changed(changed), pointer(pointer) {}

  static auto collect(Core::View::Vector<Key> keys, const Mapping& mapping)
      -> KeyBits;
  static auto set(KeyBits& keys, Key key) -> void;
  static auto contains(const KeyBits& keys, Key key) -> Bool;
  static auto add_modifier_groups(KeyBits& keys) -> void;

  KeyBits current;
  KeyBits changed;
  Pointer pointer;
};

// Three words cover the current virtual key space. Keeping the carrier within
// this ceiling makes accidental state duplication visible during review.
static_assert(Input::key_count <= 192);
static_assert(sizeof(Input) <= 80);

}  // namespace Perimortem::System
