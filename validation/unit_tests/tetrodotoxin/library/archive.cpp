// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/archive/writer.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness LibraryArchive = {
  .name = "Tetrodotoxin::Library::Archive"_view,
};

PERIMORTEM_UNIT_TEST(LibraryArchive, frozen_format) {
  static constexpr Static::Vector<Library::Archive::Tag, 31> tags = {{
    Library::Archive::Tag::Source,
    Library::Archive::Tag::Import,
    Library::Archive::Tag::Foreign,
    Library::Archive::Tag::ForeignState,
    Library::Archive::Tag::ForeignFunction,
    Library::Archive::Tag::Alias,
    Library::Archive::Tag::Structure,
    Library::Archive::Tag::Object,
    Library::Archive::Tag::Enumeration,
    Library::Archive::Tag::EnumerationCase,
    Library::Archive::Tag::Field,
    Library::Archive::Tag::Function,
    Library::Archive::Tag::Signature,
    Library::Archive::Tag::TypeReference,
    Library::Archive::Tag::PackGroup,
    Library::Archive::Tag::ConstantFalse,
    Library::Archive::Tag::ConstantTrue,
    Library::Archive::Tag::ConstantUnsigned,
    Library::Archive::Tag::ConstantSigned,
    Library::Archive::Tag::ConstantReal,
    Library::Archive::Tag::ConstantBytes,
    Library::Archive::Tag::ConstantEnumeration,
    Library::Archive::Tag::ConstantRange,
    Library::Archive::Tag::ConstantOption,
    Library::Archive::Tag::ConstantResult,
    Library::Archive::Tag::Layout,
    Library::Archive::Tag::ConstantObject,
    Library::Archive::Tag::ConstantResourceBytes,
    Library::Archive::Tag::Namespace,
    Library::Archive::Tag::Interface,
    Library::Archive::Tag::Implemented,
  }};
  static constexpr Static::Vector<U16, 31> golden = {{
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  }};

  for (Count index = 0; index < tags.get_size(); index++) {
    EXPECT_EQ(U16(tags[index]), golden[index]);
  }

  static constexpr Static::Vector<U8, 16> framing = {{
    0x54,
    0x54,
    0x58,
    0x4C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x01,
    0x00,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
  }};

  Library::Archive::Writer writer;
  auto record = writer.begin(Library::Archive::Tag::Source, True);
  ASSERT(writer.finish(record));
  auto bytes = writer.take();
  EXPECT_HEX(bytes.get_view(), framing.get_view().get_bytes());
}
