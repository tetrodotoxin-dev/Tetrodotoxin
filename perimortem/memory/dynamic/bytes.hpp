// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/hash.hpp"
#include "perimortem/core/object.hpp"

namespace Perimortem::Memory::Dynamic {

// Bytes owns copy on write value policy over one worker local Object buffer.
// Object itself preserves ordinary reference identity while Bytes alone decides
// when a shared allocation must be copied before mutation.
class Bytes {
 public:
  constexpr Bytes() = default;
  Bytes(Count reserved_capacity);
  Bytes(Core::View::Bytes view);
  Bytes(const Dynamic::Bytes& rhs);
  Bytes(Dynamic::Bytes&& rhs);

  auto operator=(Core::View::Bytes view) -> Dynamic::Bytes&;
  auto operator=(const Dynamic::Bytes& rhs) -> Dynamic::Bytes&;
  auto operator=(Dynamic::Bytes&& rhs) -> Dynamic::Bytes&;

  auto operator==(const Dynamic::Bytes& rhs) const -> Bool {
    return get_view() == rhs.get_view();
  }

  auto operator==(const Core::View::Bytes& rhs) const -> Bool {
    return get_view() == rhs;
  }

  ~Bytes() = default;

  operator Core::View::Bytes() const { return get_view(); }
  operator Core::Access::Bytes() { return get_access(); }

  auto append(U8 byte) -> void;
  auto append(U8 byte, Count amount) -> void;
  auto concat(Core::View::Bytes view) -> void;
  auto proxy(Core::View::Bytes view) -> void;
  // Resizes the container but attempts to preserve as much of the original
  // buffer as will fit in the new size.
  //
  // Shrinking preserves the remaining buffer so restoring the old size can
  // recover its bytes.
  auto resize(Count new_size) -> void;
  // Ensures there is enough room to store a required size, but declares we
  // don't care about the buffer's existing contents.
  //
  // Both growing and shrinking the buffer can be destructive operations so the
  // contents after a forgetful operation should always be assumed to be in an
  // invalid state.
  auto forgetful_resize(Count required_size) -> void;
  // Shrinks the container from the front by a number of bytes.
  //
  // If the container is shrunk more than it's current size the call is
  // equivilant to a clear.
  auto shrink(Count bytes_to_remove) -> void;

  auto operator[](Count index) const -> U8;
  auto at(Count index) const -> U8;
  auto set(U8 target) -> void;
  auto convert(U8 source, U8 target) -> void;
  auto slice(Count start, Count size) const -> Core::View::Bytes;

  constexpr auto get_size() const -> Count { return size; }
  auto get_capacity() const -> Count { return data.get_capacity(); }
  auto get_view() const -> Core::View::Bytes {
    return Core::View::Bytes(data.get_view().get_data(), size);
  }

  // Access promises writable Bytes value storage, so Bytes detaches a shared
  // Object before its address escapes. The returned bounds do not extend the
  // Bytes lifetime.
  auto get_access() -> Core::Access::Bytes;

  auto hash() const -> U64 { return Core::Hash(get_view()).get_value(); }

  constexpr auto is_empty() const -> Bool { return size == 0; }

  auto clear() -> void;
  auto reset() -> void;
  auto ensure_capacity(Count required_size) -> void;

 private:
  auto prepare_write(Count required_capacity) -> Core::Access::Bytes;

  Core::Object<U8> data;
  Count size = 0;
};

static_assert(sizeof(Bytes) == sizeof(U8*) + sizeof(Count));
static_assert(alignof(Bytes) == alignof(Count));

}  // namespace Perimortem::Memory::Dynamic
