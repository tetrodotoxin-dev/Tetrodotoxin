// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/data/encoding/block.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/const/vector.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Data::Encoding;

static Validation::Harness EncodingBlock = {
  .name = "TTX::Data::Encoding::Block"_view};

// This oracle reads individual bits rather than using the chunk algorithm.
// Arbitrary bytes exercise both zero and nonzero high fields without relying
// on the compiler or encoder to produce the expected answer.
static constexpr auto bits(View::Bytes bytes, Count first, Count width)
    -> Count {
  Count result = 0;
  for (Count bit = 0; bit < width && bit < 64; ++bit) {
    const Count position = first + bit;
    const Count value = (bytes[position >> 3] >> (position & 7)) & 1;
    result |= value << bit;
  }

  return result;
}

static constexpr U8 constant[] = {0x81, 0x42, 0x24, 0x18, 0xf0, 0x0f,
                                  0xaa, 0x55, 0x96, 0x69, 0x87, 0x78,
                                  0,    0,    0,    0};
static_assert(
    Block::extract(View::Bytes(constant), 61, 35) ==
    bits(View::Bytes(constant), 61, 35));

PERIMORTEM_UNIT_TEST(EncodingBlock, field_boundaries) {
  const Count widths[] = {1, 6, 7, 8, 12, 16, 31, 32, 33, 63, 64, 72, 120};
  for (U8 depth = 1; depth <= 15; ++depth) {
    const Count block_size = Count(depth) << 2;
    for (Count count = 2; count <= 3; ++count) {
      const Count raw_size = count * block_size;
      const Count size = Data::align<8>(raw_size);

      // Two blocks cover an already complete buffer; three require padding
      // at odd depths. The one-byte prefix misaligns native memory, while each
      // U64 read must still use coordinates from the beginning of the buffer.
      // The exact allocation lets ASan catch a read past its padded end.
      Perimortem::Memory::Const::Vector<U8> storage;
      storage.resize(size + 1);
      for (Count i = 0; i < raw_size; ++i) {
        storage[i + 1] = U8(i * 37 + 11);
      }

      const View::Bytes bytes(storage.get_data() + 1, size);
      for (Count first = 0; first < raw_size * 8; ++first) {
        for (const Count width : widths) {
          if (width <= raw_size * 8 - first) {
            ASSERT_EQ(
                Block::extract(bytes, first, width), bits(bytes, first, width));
          }
        }
      }
    }
  }
}
