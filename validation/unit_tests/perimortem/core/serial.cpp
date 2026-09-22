// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/reader/serial.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/serial.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness CoreSerialReader = {
  .name = "Core::Reader::Serial"_view,
};

PERIMORTEM_UNIT_TEST(CoreSerialReader, regular_values) {
  Static::Bytes<19> buffer =
      "\x01\xAB"
      "\x02\x34\x12"
      "\x04"
      "IREP"  // Stream is always little endian
      "\x08\xEF\xCD\xAB\x89\x67\x45\x23\x01"_view;
  Reader::Serial reader(buffer);

  EXPECT_EQ(reader.read_value(), 0xAB);
  EXPECT_EQ(reader.read_value(), 0x1234);
  EXPECT_EQ(reader.read_value(), 'PERI');
  EXPECT_EQ(reader.read_value(), S64(0x0123456789ABCDEF));
  EXPECT_EQ(reader.get_location(), reader.get_size());
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, negative_values) {
  Static::Bytes<21> buffer =
      "\x11\xAB"
      "\x12\x34\x12"
      "\x14"
      "IREP"  // Stream is always little endian
      "\x18\xEF\xCD\xAB\x89\x67\x45\x23\x01"
      "\x11\x64"_view;
  Reader::Serial reader(buffer);

  EXPECT_EQ(reader.read_value(), -0xAB);
  EXPECT_EQ(reader.read_value(), -0x1234);
  EXPECT_EQ(reader.read_value(), -'PERI');
  EXPECT_EQ(reader.read_value(), S64(-0x0123456789ABCDEF));
  EXPECT_EQ(reader.read_value(), 0xFFFFFFFFFFFFFF9C);
  EXPECT_EQ(reader.get_location(), reader.get_size());
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, small_blobs) {
  Static::Bytes<30> buffer =
      "\x21\x0A"
      "Perimortem"
      "\x01\x04"
      "\x21\x0E"
      "Testing String"_view;
  Reader::Serial reader(buffer);

  EXPECT_EQ(reader.read_blob(), "Perimortem"_view);
  EXPECT_EQ(reader.read_value(), 4);
  EXPECT_EQ(reader.read_blob(), "Testing String"_view);
  EXPECT_EQ(reader.get_location(), reader.get_size());
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, large_blob) {
  Static::Bytes<500> expected([](Count i) -> U8 { return i; });
  Static::Bytes<503> source([](Count i) -> U8 {
    // Header
    if (i < 3) {
      return "\x22\xF4\x01"_view[i];
    }

    return (i - 3);
  });
  Reader::Serial reader(source);

  EXPECT_HEX(reader.read_blob(), expected.get_view());
  EXPECT_EQ(reader.get_location(), reader.get_size());
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, only_values) {
  Static::Bytes<30> buffer =
      "\x21\x0A"
      "Perimortem"
      "\x01\x04"
      "\x21\x0E"
      "Testing String"_view;
  Reader::Serial reader(buffer);

  EXPECT_EQ(reader.read_value(), 0);
  EXPECT_EQ(reader.read_value(), 4);
  EXPECT_EQ(reader.read_value(), 0);
  EXPECT_EQ(reader.get_location(), reader.get_size());
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, only_blobs) {
  Static::Bytes<30> buffer =
      "\x21\x0A"
      "Perimortem"
      "\x01\x04"
      "\x21\x0E"
      "Testing String"_view;
  Reader::Serial reader(buffer);

  EXPECT_EQ(reader.read_blob(), "Perimortem"_view);
  EXPECT_EQ(reader.read_blob(), ""_view);
  EXPECT_EQ(reader.read_blob(), "Testing String"_view);
  EXPECT_EQ(reader.get_location(), reader.get_size());
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, type_mismatch) {
  Reader::Serial reader("\x18\xAD\xDE\x00\x00"_view);
  auto scope_attribution = Diagnostics::Log::set_attribution();

  EXPECT_EQ(reader.read_value(), 0);
  EXPECT_EQ(reader.get_location(), Count(-1));

  // Make sure message was logged.
  constexpr auto error_message =
      "Serial read overran data buffer while reading value at byte location 1. source_size=5 encoded_size=8"_view;
  EXPECT(Test::error_contains(error_message));
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, bad_encoding) {
  Reader::Serial reader("\x13\xAD\xDE\x00\x00"_view);
  auto scope_attribution = Diagnostics::Log::set_attribution();

  EXPECT_EQ(reader.read_value(), 0);
  EXPECT_EQ(reader.get_location(), Count(-1));

  // Make sure message was logged.
  constexpr auto error_message =
      "Serial read found invalid encoding size 3 at byte location 1."_view;
  EXPECT(Test::error_contains(error_message));
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, bad_blob) {
  Reader::Serial reader("\x24\xAD\xDE\x00\x00"_view);
  auto scope_attribution = Diagnostics::Log::set_attribution();

  EXPECT_EQ(reader.read_value(), 0);
  EXPECT_EQ(reader.get_location(), Count(-1));

  // Make sure message was logged.
  constexpr auto error_message =
      "Serial read of blob sized 57005 bytes overran source buffer at location "
      "5. source_size=5 blob_size=57005"_view;
  EXPECT(Test::error_contains(error_message));
}

PERIMORTEM_UNIT_TEST(CoreSerialReader, read_from_empty) {
  Reader::Serial reader(""_view);
  auto scope_attribution = Diagnostics::Log::set_attribution();

  EXPECT_EQ(reader.read_value(), 0);
  EXPECT_EQ(reader.get_location(), Count(-1));

  // Make sure message was logged.
  constexpr auto error_message =
      "Serial read overran data buffer while reading type at byte location 0."_view;
  EXPECT(Test::error_contains(error_message));
}

static Harness CoreSerialWriter = {
  .name = "Core::Writer::Serial"_view,
};

PERIMORTEM_UNIT_TEST(CoreSerialWriter, regular_values) {
  Static::Bytes<19> buffer;
  Writer::Serial writer(buffer);

  writer << 0xAB << 0x1234 << 'PERI' << 0x0123456789ABCDEF;

  EXPECT(writer.is_valid());
  EXPECT_EQ(writer.get_location(), 19);
  EXPECT_HEX(
      buffer,
      "\x01\xAB"
      "\x02\x34\x12"
      "\x04"
      "IREP"  // Stream is always little endian
      "\x08\xEF\xCD\xAB\x89\x67\x45\x23\x01"_view);
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, negative_values) {
  Static::Bytes<21> buffer;
  Writer::Serial writer(buffer);

  // Negated values should resolve to their unsigned version and store their
  // negate flag.
  writer << -0xAB << -0x1234 << -'PERI' << -0x0123456789ABCDEF;

  // Negative values should compress to their smaller unsigned representation.
  // 0xFFFFFFFFFFFFFF9C gets converted to 100 and uses only 1 byte for the value
  // while storing the negate flag (0x10).
  writer << /* negative 100 */ 0xFFFFFFFFFFFFFF9C;

  EXPECT(writer.is_valid());
  EXPECT_HEX(
      buffer,
      "\x11\xAB"
      "\x12\x34\x12"
      "\x14"
      "IREP"  // Stream is always little endian
      "\x18\xEF\xCD\xAB\x89\x67\x45\x23\x01"
      "\x11\x64"_view);
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, small_blobs) {
  Static::Bytes<30> buffer;
  Writer::Serial writer(buffer);

  writer << "Perimortem"_view << 4 << "Testing String"_view;

  EXPECT(writer.is_valid());
  EXPECT_HEX(
      buffer,
      "\x21\x0A"
      "Perimortem"
      "\x01\x04"
      "\x21\x0E"
      "Testing String"_view);
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, large_blob) {
  Static::Bytes<500> source([](Count i) -> U8 { return i; });
  Static::Bytes<503> expected([](Count i) -> U8 {
    // Header
    if (i < 3) {
      return "\x22\xF4\x01"_view[i];
    }

    return (i - 3);
  });
  Static::Bytes<expected.get_size()> buffer;
  Writer::Serial writer(buffer);

  writer << source;

  EXPECT(writer.is_valid());
  EXPECT_HEX(buffer, expected);
}

PERIMORTEM_UNIT_TEST(CoreSerialWriter, overflow) {
  Static::Bytes<20> buffer;
  Writer::Serial writer(buffer);

  writer << "Perimortem"_view << 4 << "Testing String"_view;

  EXPECT_NOT(writer.is_valid());
}
