// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/reader/textual.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Validation;

static Harness CoreTextualReader = {
  .name = "Core::Reader::Textual"_view,
};

PERIMORTEM_UNIT_TEST(CoreTextualReader, integers) {
  Reader::Textual reader(
      "-1234 5678 -99999 100000 -1234567890123 9876543210"_view);

  EXPECT(reader.has_content());
  EXPECT_EQ(reader.read_signed(), S64(-1234));
  EXPECT_EQ(reader.read_signed(), S64(5678));
  EXPECT_EQ(reader.read_signed(), S64(-99999));
  EXPECT_EQ(reader.read_signed(), S64(100000));
  EXPECT_EQ(reader.read_signed(), S64(-1234567890123LL));
  EXPECT_EQ(reader.read_signed(), S64(9876543210ULL));
  EXPECT(reader.is_valid());
  EXPECT_EQ(reader.get_location(), reader.get_size());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, integer_radix_limits) {
  Reader::Textual unsigned_maximum("18446744073709551615"_view);
  EXPECT_EQ(unsigned_maximum.read_unsigned(), U64(-1));
  EXPECT(unsigned_maximum.is_valid());
  EXPECT_EQ(unsigned_maximum.get_location(), unsigned_maximum.get_size());

  Reader::Textual signed_limits(
      "-9223372036854775808 9223372036854775807"_view);
  EXPECT_EQ(signed_limits.read_signed(), S64(-9223372036854775807LL - 1));
  EXPECT_EQ(signed_limits.read_signed(), S64(9223372036854775807LL));
  EXPECT(signed_limits.is_valid());
  EXPECT_EQ(signed_limits.get_location(), signed_limits.get_size());

  Reader::Textual hexadecimal("FFFFFFFFFFFFFFFF"_view);
  EXPECT_EQ(hexadecimal.read_unsigned(16), U64(-1));
  EXPECT(hexadecimal.is_valid());
  EXPECT_EQ(hexadecimal.get_location(), hexadecimal.get_size());

  Reader::Textual unsigned_overflow("18446744073709551616"_view);
  EXPECT_EQ(unsigned_overflow.read_unsigned(), U64(0));
  EXPECT_NOT(unsigned_overflow.is_valid());

  Reader::Textual positive_overflow("9223372036854775808"_view);
  EXPECT_EQ(positive_overflow.read_signed(), S64(0));
  EXPECT_NOT(positive_overflow.is_valid());

  Reader::Textual negative_overflow("-9223372036854775809"_view);
  EXPECT_EQ(negative_overflow.read_signed(), S64(0));
  EXPECT_NOT(negative_overflow.is_valid());

  Reader::Textual hexadecimal_overflow("10000000000000000"_view);
  EXPECT_EQ(hexadecimal_overflow.read_unsigned(16), U64(0));
  EXPECT_NOT(hexadecimal_overflow.is_valid());

  Reader::Textual invalid_radix("10"_view);
  EXPECT_EQ(invalid_radix.read_unsigned(1), U64(0));
  EXPECT_NOT(invalid_radix.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, integers_and_text) {
  Reader::Textual reader("count: 412010 items"_view);

  EXPECT_EQ(reader.read_byte(), U8('c'));
  reader.read_byte();  // 'o'
  reader.read_byte();  // 'u'
  reader.read_byte();  // 'n'
  EXPECT_EQ(reader.read_byte(), U8('t'));
  reader.read_byte();  // ':'
  EXPECT_EQ(reader.read_unsigned(), U64(412010));
  EXPECT(reader.has_content());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, boolean) {
  // C++ native bool narrows to int and prints as 1/0 so make sure we test for
  // that for ABI reasons, and use Bool (proper ABI) for "true"/"false".
  Reader::Textual reader("10truefalseTrueFalse"_view);

  EXPECT_EQ(reader.read_byte(), U8('1'));
  EXPECT_EQ(reader.read_byte(), U8('0'));
  EXPECT(reader.read_flag());
  EXPECT_NOT(reader.read_flag());
  EXPECT(reader.read_flag());
  EXPECT_NOT(reader.read_flag());
  EXPECT_NOT(reader.has_content());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, floats) {
  Reader::Textual reader("12.5 -1.5 2.512 0.0 -0.5"_view);

  EXPECT_EQ(reader.read_r64(), R64(12.5));
  EXPECT_EQ(reader.read_r64(), R64(-1.5));
  EXPECT_EQ(reader.read_r32(), R32(2.512f));
  EXPECT_EQ(reader.read_r64(), R64(0.0));
  EXPECT_EQ(reader.read_r64(), R64(-0.5));
  EXPECT(reader.is_valid());
  EXPECT_EQ(reader.get_location(), reader.get_size());
  EXPECT_NOT(reader.has_content());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, real_limits) {
  constexpr auto wide_text = "999999999999999999999999999999999999999.0"_view;
  Reader::Textual wide(wide_text);
  EXPECT(wide.read_r64() > R64(1e38));
  EXPECT(wide.is_valid());
  EXPECT_EQ(wide.get_location(), wide.get_size());

  Reader::Textual wide_32(wide_text);
  EXPECT_EQ(wide_32.read_r32(), R32(0));
  EXPECT_NOT(wide_32.is_valid());

  constexpr auto tiny_text =
      "0.000000000000000000000000000000000000000000000000001"_view;
  Reader::Textual tiny(tiny_text);
  EXPECT(tiny.read_r64() > R64(0));
  EXPECT(tiny.is_valid());
  EXPECT_EQ(tiny.get_location(), tiny.get_size());

  Reader::Textual tiny_32(tiny_text);
  EXPECT_EQ(tiny_32.read_r32(), R32(0));
  EXPECT_NOT(tiny_32.is_valid());

  Reader::Textual overflow(
      "9999999999999999999999999999999999999999999999999999999999999999"
      "9999999999999999999999999999999999999999999999999999999999999999"
      "9999999999999999999999999999999999999999999999999999999999999999"
      "9999999999999999999999999999999999999999999999999999999999999999"
      "9999999999999999999999999999999999999999999999999999999999999999.0"_view);
  EXPECT_EQ(overflow.read_r64(), R64(0));
  EXPECT_NOT(overflow.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, prevent_overflow) {
  Reader::Textual reader("Hello"_view);
  for (Count i = 0; i < 5; i++) {
    reader.read_byte();
  }

  EXPECT_NOT(reader.has_content());

  reader.read_byte();
  EXPECT_NOT(reader.has_content());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, invalid_bool) {
  Reader::Textual reader("badflag"_view);
  reader.read_flag();
  EXPECT_NOT(reader.has_content());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, just_whitespace) {
  Reader::Textual reader("      "_view);

  reader.read_byte();
  EXPECT(reader.has_content());
  reader.reset();
  reader.read_signed();
  EXPECT_NOT(reader.has_content());
  reader.reset();
  reader.read_unsigned();
  EXPECT_NOT(reader.has_content());
  reader.reset();
  reader.read_r32();
  EXPECT_NOT(reader.has_content());
  reader.reset();
  reader.read_r64();
  EXPECT_NOT(reader.has_content());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, set_location) {
  Reader::Textual reader("42 99"_view);

  EXPECT_EQ(reader.read_signed(), 42);
  EXPECT(reader.has_content());
  EXPECT_EQ(reader.read_unsigned(), 99);
  EXPECT_NOT(reader.has_content());

  reader.set_location(0);
  EXPECT(reader.has_content());
  EXPECT_EQ(reader.read_unsigned(), 42);
  EXPECT(reader.has_content());
  EXPECT_EQ(reader.read_signed(), 99);
  EXPECT_NOT(reader.has_content());

  reader.set_location(reader.get_size() + 1);
  EXPECT_NOT(reader.is_valid());
}

PERIMORTEM_UNIT_TEST(CoreTextualReader, multiple_readers) {
  // Both readers operate independently over the same source.
  View::Bytes source = "a12 true"_view;
  Reader::Textual readers[] = {
    Reader::Textual(source), Reader::Textual(source)};

  EXPECT_EQ(readers[0].read_byte(), readers[1].read_byte());
  EXPECT_EQ(readers[0].get_location(), Count(1));
  EXPECT_EQ(readers[1].get_location(), Count(1));

  EXPECT_EQ(readers[0].read_signed(), 12);
  EXPECT_EQ(readers[0].read_flag(), true);

  EXPECT_NOT(readers[0].has_content());
  EXPECT(readers[1].has_content());

  EXPECT_EQ(readers[1].read_unsigned(), 12);
  EXPECT_EQ(readers[1].read_flag(), true);

  EXPECT_NOT(readers[0].has_content());
  EXPECT_NOT(readers[1].has_content());
}

static Harness CoreTextual = {
  .name = "Core::Writer::Textual"_view,
};

PERIMORTEM_UNIT_TEST(CoreTextual, simple_text) {
  Static::Bytes<10> buffer;
  Writer::Textual writer(buffer.get_access());

  writer << "Hello"_view << "World"_view;

  EXPECT(writer.is_valid());
  EXPECT_TEXT(buffer, "HelloWorld"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, prevent_overflow) {
  Static::Bytes<10> buffer;
  Writer::Textual writer(buffer.get_access());

  writer << "Hello"_view << "World!!!!"_view;

  EXPECT_NOT(writer.is_valid());
  EXPECT_TEXT(buffer.slice(0, 5), "Hello"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, integers) {
  Static::Bytes<12> buffer;
  Writer::Textual writer(buffer.get_access());

  writer << 120000 << -31 << 208;

  EXPECT(writer.is_valid());
  EXPECT_TEXT(buffer, "120000-31208"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, characters) {
  Static::Bytes<8> buffer;
  Writer::Textual writer(buffer.get_access());

  writer << S8(-128) << ':' << U8(255);

  EXPECT(writer.is_valid());
  EXPECT_TEXT(buffer, "-128:255"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, integers_and_text) {
  Static::Bytes<24> buffer;
  Writer::Textual writer(buffer.get_access());

  writer << "Test Value: "_view << 412010 << " units"_view;

  EXPECT(writer.is_valid());
  EXPECT_TEXT(buffer, "Test Value: 412010 units"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, boolean) {
  Static::Bytes<11> buffer;
  Writer::Textual writer(buffer.get_access());

  // C++ will narrow to int and should print 1 and 0
  writer << true << false;

  // Perimortem types print out the full value
  writer << True << False;

  EXPECT(writer.is_valid());
  EXPECT_TEXT(buffer, "10truefalse"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, floats) {
  Static::Bytes<40> buffer;
  Writer::Textual writer(buffer.get_access());

  writer << 12.08 << 0.0000812;

  // Percision test
  writer << R32(-2012.78102) << R64(-2012.78102);

  EXPECT(writer.is_valid());
  EXPECT_TEXT(
      buffer.get_view(), "12.080.0000812-2012.7810058-2012.78102\0\0"_view);
}

PERIMORTEM_UNIT_TEST(CoreTextual, multiple_writers) {
  Static::Bytes<34> buffer;
  for (Count index = 0; index < buffer.get_size(); index++) {
    buffer[index] = '?';
  }

  Writer::Textual writers[] = {
    Writer::Textual(buffer.get_access()),
    Writer::Textual(buffer.get_access()),
  };

  writers[0] << "Sample text for testing!"_view;
  ASSERT_TEXT(buffer, "Sample text for testing!??????????"_view);
  writers[1] << "test: "_view << True;
  ASSERT_TEXT(buffer, "test: truet for testing!??????????"_view);
  writers[0] << " "_view << 13891;
  ASSERT_TEXT(buffer, "test: truet for testing! 13891????"_view);
  writers[1] << 79.8106;
  ASSERT_TEXT(buffer, "test: true79.8106esting! 13891????"_view);
  writers[0] << "Too much text!!"_view;
  ASSERT_TEXT(buffer, "test: true79.8106esting! 13891????"_view);

  EXPECT_NOT(writers[0].is_valid());
  EXPECT(writers[1].is_valid());
}
