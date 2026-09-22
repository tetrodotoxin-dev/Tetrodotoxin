// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/compression/deflate.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem;

using namespace Validation;

static Harness CompressionTests = {
  .name = "Compression"_view,
};

// Known test blobs produced by zlib

// "Hello, Perimortem!"
static constexpr Static::Bytes<26> hello_compressed = {{
  0x78, 0xDA, 0xF3, 0x48, 0xCD, 0xC9, 0xC9, 0xD7, 0x51, 0x08, 0x48, 0x2D, 0xCA,
  0xCC, 0xCD, 0x2F, 0x2A, 0x49, 0xCD, 0x55, 0x04, 0x00, 0x3D, 0x2E, 0x06, 0x86,
}};
static constexpr Static::Bytes<18> hello_raw = {{
  0x48,
  0x65,
  0x6C,
  0x6C,
  0x6F,
  0x2C,
  0x20,
  0x50,
  0x65,
  0x72,
  0x69,
  0x6D,
  0x6F,
  0x72,
  0x74,
  0x65,
  0x6D,
  0x21,
}};

// "Stored block test data ABCDEFGHIJ" — level 0
static constexpr Static::Bytes<44> stored_compressed = {{
  0x78, 0x01, 0x01, 0x21, 0x00, 0xDE, 0xFF, 0x53, 0x74, 0x6F, 0x72,
  0x65, 0x64, 0x20, 0x62, 0x6C, 0x6F, 0x63, 0x6B, 0x20, 0x74, 0x65,
  0x73, 0x74, 0x20, 0x64, 0x61, 0x74, 0x61, 0x20, 0x41, 0x42, 0x43,
  0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0xC9, 0x70, 0x0B, 0x0E,
}};
static constexpr Static::Bytes<33> stored_raw = {{
  0x53, 0x74, 0x6F, 0x72, 0x65, 0x64, 0x20, 0x62, 0x6C, 0x6F, 0x63,
  0x6B, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x64, 0x61, 0x74, 0x61,
  0x20, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
}};

// 0xAB 0xCD repeated 50 times
static constexpr Static::Bytes<13> repeat_compressed = {{
  0x78,
  0xDA,
  0x5B,
  0x7D,
  0x76,
  0x35,
  0xCD,
  0x21,
  0x00,
  0x7A,
  0x7C,
  0x49,
  0x71,
}};

// Empty input
static constexpr Static::Bytes<8> empty_compressed = {{
  0x78,
  0xDA,
  0x03,
  0x00,
  0x00,
  0x00,
  0x00,
  0x01,
}};

// Single byte 0x42
static constexpr Static::Bytes<9> single_compressed = {{
  0x78,
  0xDA,
  0x73,
  0x02,
  0x00,
  0x00,
  0x43,
  0x00,
  0x43,
}};

// hello_compressed with the last byte of its checksum flipped
#if PERI_DEBUG
static constexpr Static::Bytes<26> bad_adler_compressed = {{
  0x78, 0xDA, 0xF3, 0x48, 0xCD, 0xC9, 0xC9, 0xD7, 0x51, 0x08, 0x48, 0x2D, 0xCA,
  0xCC, 0xCD, 0x2F, 0x2A, 0x49, 0xCD, 0x55, 0x04, 0x00, 0x3D, 0x2E, 0x06, 0x79,
}};
#endif

PERIMORTEM_UNIT_TEST(CompressionTests, dynamic_huffman) {
  auto out = Compression::Deflate::inflate(hello_compressed);

  ASSERT_EQ(out.get_size(), hello_raw.get_size());
  EXPECT_HEX(out.get_view(), hello_raw.get_view());
}

PERIMORTEM_UNIT_TEST(CompressionTests, stored_blocks) {
  auto out = Compression::Deflate::inflate(stored_compressed);

  ASSERT_EQ(out.get_size(), stored_raw.get_size());
  EXPECT_HEX(out.get_view(), stored_raw.get_view());
}

PERIMORTEM_UNIT_TEST(CompressionTests, back_references) {
  auto out = Compression::Deflate::inflate(repeat_compressed);

  // Check that ABCD bytes are repeated 50 times.
  ASSERT_EQ(out.get_size(), 100);
  for (Count i = 0; i < 100; i += 2) {
    EXPECT_EQ(out[i + 0], U8(0xAB));
    EXPECT_EQ(out[i + 1], U8(0xCD));
  }
}

PERIMORTEM_UNIT_TEST(CompressionTests, empty_content) {
  auto out = Compression::Deflate::inflate(empty_compressed);
  EXPECT_EQ(out.get_size(), 0);
}

PERIMORTEM_UNIT_TEST(CompressionTests, inflate_single_byte) {
  auto out = Compression::Deflate::inflate(single_compressed);

  ASSERT_EQ(out.get_size(), 1);
  EXPECT_EQ(out[0], U8(0x42));
}

PERIMORTEM_UNIT_TEST(CompressionTests, truncated_input) {
  // Exercise the boundary immediately below the minimum zlib stream size.
  auto out = Compression::Deflate::inflate(hello_compressed.slice(0, 6));
  EXPECT_EQ(out.get_size(), 0);
  EXPECT(
      Test::error_contains(
          "Compression: Input too short to be a valid deflate stream"_view));
}

PERIMORTEM_UNIT_TEST(CompressionTests, bad_method) {
  Static::Bytes<26> bad_cm = hello_compressed;

  // Corrupt the CM nibble to 9 (DEFLATE requires exactly 8).
  bad_cm[0] = (bad_cm[0] & 0xF0) | 0x09;

  auto out = Compression::Deflate::inflate(bad_cm);
  EXPECT_EQ(out.get_size(), 0);
  EXPECT(
      Test::error_contains(
          "Compression: Unsupported compression method in deflate header"_view));
}

#if PERI_DEBUG
PERIMORTEM_UNIT_TEST(CompressionTests, inflate_bad_checksum) {
  auto out = Compression::Deflate::inflate(bad_adler_compressed);
  EXPECT_EQ(out.get_size(), 0);
  EXPECT(Test::error_contains("Compression: Adler-32 checksum mismatch."_view));
}
#endif

PERIMORTEM_UNIT_TEST(CompressionTests, deflate_empty_input) {
  auto compressed = Compression::Deflate::deflate(""_view);

  // A valid zlib stream must still have a header and a footer.
  EXPECT(compressed.get_size() >= 6);

  // Round tripping inflate should properly produce an empty buffer.
  auto recovered = Compression::Deflate::inflate(compressed);
  EXPECT_EQ(recovered.get_size(), 0);
}

PERIMORTEM_UNIT_TEST(CompressionTests, deflate_single_byte) {
  constexpr Static::Bytes<1> source_bytes = {{
    0x42,
  }};
  auto compressed = Compression::Deflate::deflate(source_bytes);
  EXPECT(compressed.get_size() > 0);

  auto recovered = Compression::Deflate::inflate(compressed);
  ASSERT_EQ(recovered.get_size(), Count(1));
  EXPECT_EQ(recovered[0], U8(0x42));
}

PERIMORTEM_UNIT_TEST(CompressionTests, roundtrip_short) {
  auto compressed = Compression::Deflate::deflate(hello_raw);
  ASSERT(compressed.get_size() > 0);

  auto recovered = Compression::Deflate::inflate(compressed);
  ASSERT_EQ(recovered.get_size(), hello_raw.get_size());
  EXPECT_HEX(recovered.get_view(), hello_raw.get_view());
}

PERIMORTEM_UNIT_TEST(CompressionTests, roundtrip_binary) {
  // Binary data with all 256 byte values present.
  Static::Bytes<256> all_bytes;
  for (Count i = 0; i < 256; i++) {
    all_bytes[i] = U8(i);
  }

  auto compressed = Compression::Deflate::deflate(all_bytes);
  ASSERT(compressed.get_size() > 0);

  auto recovered = Compression::Deflate::inflate(compressed);
  ASSERT_EQ(recovered.get_size(), 256);
  EXPECT_HEX(recovered.get_view(), all_bytes.get_view());
}

PERIMORTEM_UNIT_TEST(CompressionTests, valid_header) {
  auto compressed = Compression::Deflate::deflate(stored_raw);

  ASSERT(compressed.get_size() >= 6);
  EXPECT_EQ(U8(compressed[0] & 0x0F), U8(8));
  EXPECT_EQ((U32(compressed[0]) * 256 + compressed[1]) % 31, U32(0));
}

PERIMORTEM_UNIT_TEST(CompressionTests, repeating_value) {
  constexpr Count source_size = 512;
  Static::Bytes<source_size> source;
  for (Count i = 0; i < source_size; i++) {
    source[i] = U8(0xAA);
  }

  auto compressed = Compression::Deflate::deflate(source);
  ASSERT(compressed.get_size() > 0);

  auto recovered = Compression::Deflate::inflate(compressed);
  ASSERT_EQ(recovered.get_size(), source_size);
  EXPECT_HEX(recovered.get_view(), source.get_view());
}

PERIMORTEM_UNIT_TEST(CompressionTests, roundtrip_large) {
  // 8 KB of structured data spanning multiple stored blocks in deflate output.
  constexpr Count size = 8192;
  Static::Bytes<size> large;
  for (Count i = 0; i < size; i++) {
    large[i] = U8((i * 31 + i / 128) & 0xFF);
  }

  auto compressed = Compression::Deflate::deflate(large);
  ASSERT(compressed.get_size() > 0);

  auto recovered = Compression::Deflate::inflate(compressed);
  ASSERT_EQ(recovered.get_size(), size);
  EXPECT_HEX(recovered.get_view(), large.get_view());
}

PERIMORTEM_UNIT_TEST(CompressionTests, skewed_frequencies) {
  constexpr Count size = 50000;
  Dynamic::Bytes source;
  source.forgetful_resize(size);
  Data::set(source.get_access().get_data(), U8(0), size);
  for (Count i = 1; i <= 200; i++) {
    source.get_access().get_data()[i * 249] = U8(i);
  }

  auto compressed = Compression::Deflate::deflate(source.get_view());
  ASSERT(compressed.get_size() > 0);

  auto recovered = Compression::Deflate::inflate(compressed);
  ASSERT_EQ(recovered.get_size(), size);
  EXPECT_HEX(recovered.get_view(), source.get_view());
}
