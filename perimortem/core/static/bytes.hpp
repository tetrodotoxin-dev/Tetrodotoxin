// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/hash.hpp"
#include "perimortem/core/math.hpp"

namespace Perimortem::Core::Static {

// Used to convert string literals into nonnull terminated bytes.
template <Count literal_size>
class Bytes {
 private:
  struct Storage {
    U8 source_block[literal_size];
  };

  Storage storage{};

 public:
  constexpr Bytes() {}

  // Static bytes initialize their complete storage from one aggregate source.
  // Call sites use an extra pair of braces, as in
  // `Static::Bytes<3> bytes = {{1, 2, 3}}`.
  constexpr Bytes(const Storage& source) : storage(source) {}

  // Initializes a buffer from a source view, either taking a slice of the data
  // or zero extending the ouput if the buffer is larger than the input.
  constexpr Bytes(const View::Bytes& source) {
    const Count size = Math::min(literal_size, source.get_size());
    if consteval {
      Count i = 0;
      for (; i < size; i++) {
        storage.source_block[i] = source.get_data()[i];
      }

      // Zero out the rest of the array so we don't expose junk data.
      for (; i < literal_size; i++) {
        storage.source_block[i] = 0;
      }
    } else {
      Data::copy(storage.source_block, source.get_data(), size);
      Data::set(storage.source_block + size, 0, literal_size - size);
    }
  }

  // Allows for generating data that would be a pain to manually write out.
  constexpr Bytes(U8 (*generator)(Count)) {
    for (Count i = 0; i < literal_size; i++) {
      storage.source_block[i] = generator(i);
    }
  }

  // Fast read function that assumes the range is valid.
  constexpr Bytes(const U8* source) {
    if consteval {
      for (Count i = 0; i < literal_size; i++) {
        storage.source_block[i] = source[i];
      }
    } else {
      if constexpr (literal_size < 16) {
        for (Count i = 0; i < literal_size; i++) {
          storage.source_block[i] = source[i];
        }
      } else {
        Data::copy(storage.source_block, source, literal_size);
      }
    }
  }

  constexpr operator View::Bytes() const { return get_view(); }
  constexpr operator Access::Bytes() { return get_access(); }

  constexpr auto operator==(const View::Bytes& rhs) -> Bool {
    if (rhs.get_size() != literal_size) {
      return False;
    }

    return Data::compare(storage.source_block, rhs.get_data(), literal_size);
  }

  constexpr auto operator!=(const View::Bytes& rhs) -> Bool {
    return !(*this == rhs);
  }

  constexpr auto operator[](Count index) -> U8& {
    return storage.source_block[index];
  }

  constexpr auto operator[](Count index) const -> const U8& {
    return storage.source_block[index];
  }

  constexpr auto slice(Count start, Count size) const -> View::Bytes {
    if (start >= get_size()) {
      return View::Bytes();
    }

    return View::Bytes(
        storage.source_block + start, Math::min(size, get_size() - start));
  }

  constexpr auto get_size() const -> Count { return literal_size; }
  constexpr auto get_capacity() const -> Count { return literal_size; }
  constexpr auto get_view() const -> const View::Bytes {
    return View::Bytes(storage.source_block, literal_size);
  }

  constexpr auto get_data() const -> const U8* { return storage.source_block; }
  constexpr auto get_data() -> U8* { return storage.source_block; }
  constexpr auto get_access() -> Access::Bytes {
    return Access::Bytes(storage.source_block, literal_size);
  }

  constexpr auto hash() const -> U64 {
    return Core::Hash(get_view()).get_value();
  }
};

}  // namespace Perimortem::Core::Static
