// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/serialization/base64.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/file.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Perimortem::System;

using namespace Validation;

static Harness SerializationBase64 = {
  .name = "Serialization::Base64"_view,
};

PERIMORTEM_UNIT_TEST(SerializationBase64, decode_empty) {
  const auto source = ""_view;
  const auto decoded_bytes = Base64::decode(source);

  EXPECT_TEXT(decoded_bytes.get_view(), ""_view);
}

PERIMORTEM_UNIT_TEST(SerializationBase64, decode_simple) {
  const auto source = "Base64 test string for Perimortem."_view;
  const auto encoded = "QmFzZTY0IHRlc3Qgc3RyaW5nIGZvciBQZXJpbW9ydGVtLg=="_view;
  const auto decoded_bytes = Base64::decode(encoded);

  EXPECT_TEXT(decoded_bytes.get_view(), source);
}

PERIMORTEM_UNIT_TEST(SerializationBase64, decode_image) {
  auto source = File::read("validation/data/pngs/perimortem_icon.png"_view);
  auto base64 =
      File::read("validation/data/base64/perimortem_icon.base64"_view);
  ASSERT(source);
  ASSERT(base64);
  ASSERT_NOT((*source).is_empty());
  ASSERT_NOT((*base64).is_empty());
  ASSERT((*base64).get_view()[(*base64).get_size() - 1] == '\n');
  const View::Bytes encoded =
      (*base64).get_view().slice(0, (*base64).get_size() - 1);

  // This binary fixture is independent of the evolving TTX source grammar.
  // Regenerate the golden encoding only when intentionally replacing the PNG.
  const auto decoded_bytes = Base64::decode(encoded);
  EXPECT(decoded_bytes.get_view() == (*source).get_view());
}

PERIMORTEM_UNIT_TEST(SerializationBase64, encode_empty) {
  const auto source = ""_view;
  const auto decoded_bytes = Base64::encode(source);

  EXPECT_TEXT(decoded_bytes.get_view(), ""_view);
}

PERIMORTEM_UNIT_TEST(SerializationBase64, encode_simple) {
  const auto source = "Base64 test string for Perimortem."_view;
  const auto encoded = "QmFzZTY0IHRlc3Qgc3RyaW5nIGZvciBQZXJpbW9ydGVtLg=="_view;
  const auto encoded_bytes = Base64::encode(source);

  EXPECT_TEXT(encoded_bytes.get_view(), encoded);
}

PERIMORTEM_UNIT_TEST(SerializationBase64, encode_image) {
  auto source = File::read("validation/data/pngs/perimortem_icon.png"_view);
  auto base64 =
      File::read("validation/data/base64/perimortem_icon.base64"_view);
  ASSERT(source);
  ASSERT(base64);
  ASSERT_NOT((*source).is_empty());
  ASSERT_NOT((*base64).is_empty());
  ASSERT((*base64).get_view()[(*base64).get_size() - 1] == '\n');
  const View::Bytes encoded =
      (*base64).get_view().slice(0, (*base64).get_size() - 1);

  // This binary fixture is independent of the evolving TTX source grammar.
  // Regenerate the golden encoding only when intentionally replacing the PNG.
  const auto encoded_bytes = Base64::encode((*source).get_view());
  EXPECT_TEXT(encoded_bytes.get_view(), encoded);
}
