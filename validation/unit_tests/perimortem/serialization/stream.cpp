// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/writer/binary.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/binary.hpp"
#include "perimortem/serialization/stream/textual.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Validation;

static Harness SerializationStream = {
  .name = "Serialization::Stream"_view,
};

PERIMORTEM_UNIT_TEST(SerializationStream, binary_endian) {
  Dynamic::Bytes output;
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> writer(output);

  writer << U16(0x1234) << U32(0xAABBCCDD) << "OK"_view;

  ASSERT_EQ(output.get_size(), Count(8));
  EXPECT_EQ(output[0], U8(0x34));
  EXPECT_EQ(output[1], U8(0x12));
  EXPECT_EQ(output[2], U8(0xDD));
  EXPECT_EQ(output[3], U8(0xCC));
  EXPECT_EQ(output[4], U8(0xBB));
  EXPECT_EQ(output[5], U8(0xAA));
  EXPECT_EQ(output[6], U8('O'));
  EXPECT_EQ(output[7], U8('K'));
}

PERIMORTEM_UNIT_TEST(SerializationStream, big_endian) {
  Dynamic::Bytes output;
  Stream::Binary<Data::ByteOrder::Big, Dynamic::Bytes> writer(output);

  writer << U16(0x1234) << U32(0xAABBCCDD);

  ASSERT_EQ(output.get_size(), Count(6));
  EXPECT_EQ(output[0], U8(0x12));
  EXPECT_EQ(output[1], U8(0x34));
  EXPECT_EQ(output[2], U8(0xAA));
  EXPECT_EQ(output[3], U8(0xBB));
  EXPECT_EQ(output[4], U8(0xCC));
  EXPECT_EQ(output[5], U8(0xDD));
}

PERIMORTEM_UNIT_TEST(SerializationStream, binary_reals) {
  Static::Bytes<12> expected;
  Writer::Binary<Data::ByteOrder::Little> expected_writer(
      expected.get_access());
  expected_writer << R32(12.5) << R64(-0.125);

  Dynamic::Bytes output;
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> writer(output);
  writer << R32(12.5) << R64(-0.125);

  EXPECT(output.get_view() == expected.get_view());
}

PERIMORTEM_UNIT_TEST(SerializationStream, binary_appends) {
  Dynamic::Bytes output("prefix"_view);
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> writer(output);

  writer << View::Bytes() << U16(0x1234);

  ASSERT_EQ(output.get_size(), Count(8));
  EXPECT_TEXT(output.slice(0, 6), "prefix"_view);
  EXPECT_EQ(output[6], U8(0x34));
  EXPECT_EQ(output[7], U8(0x12));
}

PERIMORTEM_UNIT_TEST(SerializationStream, managed_write) {
  Allocator::Arena arena;
  Managed::Bytes output(arena);
  Stream::Binary<Data::ByteOrder::Little, Managed::Bytes> writer(output);

  writer << U8(0x7F) << U16(0x1234);

  ASSERT_EQ(output.get_size(), Count(3));
  EXPECT_EQ(output[0], U8(0x7F));
  EXPECT_EQ(output[1], U8(0x34));
  EXPECT_EQ(output[2], U8(0x12));
}

PERIMORTEM_UNIT_TEST(SerializationStream, vector_write) {
  Static::Vector<U16, 3> values = {{
    U16(0x0102),
    U16(0x0304),
    U16(0x0506),
  }};
  Dynamic::Bytes output;
  Stream::Binary<Data::ByteOrder::Big, Dynamic::Bytes> writer(output);

  writer << values.get_view();

  ASSERT_EQ(output.get_size(), Count(6));
  EXPECT_EQ(output[0], U8(0x01));
  EXPECT_EQ(output[1], U8(0x02));
  EXPECT_EQ(output[2], U8(0x03));
  EXPECT_EQ(output[3], U8(0x04));
  EXPECT_EQ(output[4], U8(0x05));
  EXPECT_EQ(output[5], U8(0x06));
}

PERIMORTEM_UNIT_TEST(SerializationStream, native_vector) {
  Static::Vector<U32, 3> values = {{
    U32(0x01020304),
    U32(0x11223344),
    U32(0xAABBCCDD),
  }};
  Dynamic::Bytes output;
  Stream::Binary<Data::ByteOrder::Native, Dynamic::Bytes> writer(output);

  writer << values.get_view();

  EXPECT_EQ(output.get_size(), values.get_view().get_bytes().get_size());
  EXPECT(output.get_view() == values.get_view().get_bytes());
}

PERIMORTEM_UNIT_TEST(SerializationStream, textual_api) {
  Dynamic::Bytes output;
  Stream::Textual<Dynamic::Bytes> writer(output);

  writer << "value="_view << U32(120000) << ","_view << S32(-31) << " "_view
         << True;

  EXPECT_TEXT(output, "value=120000,-31 true"_view);
}

PERIMORTEM_UNIT_TEST(SerializationStream, textual_appends) {
  Dynamic::Bytes output("prefix="_view);
  Stream::Textual<Dynamic::Bytes> writer(output);

  writer << U8(42) << ";payload"_view;

  EXPECT_TEXT(output, "prefix=42;payload"_view);
}

PERIMORTEM_UNIT_TEST(SerializationStream, managed_textual) {
  Allocator::Arena arena;
  Managed::Bytes output(arena);
  Stream::Textual<Managed::Bytes> writer(output);

  writer << R32(12.5) << " "_view << False;

  EXPECT_TEXT(output.get_view(), "12.5 false"_view);
}
