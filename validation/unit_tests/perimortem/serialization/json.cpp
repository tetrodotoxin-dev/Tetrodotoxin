// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/serialization/json/blueprint.hpp"
#include "perimortem/serialization/json/node.hpp"
#include "perimortem/serialization/stream/textual.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Perimortem::System;

using namespace Validation;

static Harness SerializationJson = {
  .name = "Serialization::Json"_view,
};

PERIMORTEM_UNIT_TEST(SerializationJson, empty_node) {
  Json::Node empty;

  EXPECT(empty.is_null());

  EXPECT_NOT(empty.get_flag());
  EXPECT_EQ(empty.get_number(), 0);
  EXPECT_EQ(empty.get_real(), 0.0);
  EXPECT_EQ(empty.get_size(), 0);
  EXPECT_TEXT(empty.get_string(), View::Bytes());
}

PERIMORTEM_UNIT_TEST(SerializationJson, value_node) {
  Json::Node value;
  EXPECT(value.is_null());

  value = True;
  EXPECT(value.get_flag());
  EXPECT_EQ(value.get_number(), 0);
  EXPECT_EQ(value.get_real(), 0.0);
  EXPECT_TEXT(value.get_string(), View::Bytes());
  EXPECT_EQ(value.get_size(), 0);

  value = 142ll;
  EXPECT_NOT(value.get_flag());
  EXPECT_EQ(value.get_number(), 142ll);
  EXPECT_EQ(value.get_real(), 0.0);
  EXPECT_TEXT(value.get_string(), View::Bytes());
  EXPECT_EQ(value.get_size(), 0);

  value = 12.12;
  EXPECT_NOT(value.get_flag());
  EXPECT_EQ(value.get_number(), 0);
  EXPECT_EQ(value.get_real(), 12.12);
  EXPECT_TEXT(value.get_string(), View::Bytes());
  EXPECT_EQ(value.get_size(), 0);

  value = "Test View"_view;
  EXPECT_NOT(value.get_flag());
  EXPECT_EQ(value.get_number(), 0);
  EXPECT_EQ(value.get_real(), 0);
  EXPECT_TEXT(value.get_string(), "Test View"_view);
  EXPECT_EQ(value.get_size(), 9);
}

PERIMORTEM_UNIT_TEST(SerializationJson, access_never_faults) {
  Json::Node value;
  EXPECT(value.is_null());

  value = True;
  EXPECT(value["invalid"_view].is_null());
  EXPECT(value["invalid"_view]["access"_view].is_null());

  value = value["invalid"_view]["access"_view];

  EXPECT_NOT(value.get_flag());
  EXPECT_EQ(value.get_number(), 0);
  EXPECT_EQ(value.get_real(), 0.0);
  EXPECT_EQ(value.get_size(), 0);
  EXPECT_TEXT(value.get_string(), View::Bytes());
}

PERIMORTEM_UNIT_TEST(SerializationJson, access_is_value_type) {
  Json::Node value;
  EXPECT(value.is_null());

  auto new_node = value["invalid"_view];
  EXPECT(new_node.is_null());

  new_node = "valid"_view;
  EXPECT_NOT(new_node.is_null());
  EXPECT_TEXT(new_node.get_string(), "valid"_view);

  // Original value node should remain unchanged.
  EXPECT(value.is_null());
  EXPECT(value["invalid"_view].is_null());
}

PERIMORTEM_UNIT_TEST(SerializationJson, parse_values) {
  Allocator::Arena arena;
  Json::Node value;
  value.parse(arena, ""_view);
  EXPECT(value.is_null());

  value.parse(arena, "true"_view);
  EXPECT(value.get_flag());

  value.parse(arena, "false"_view);
  EXPECT_NOT(value.get_flag());

  value.parse(arena, "12"_view);
  EXPECT_EQ(value.get_number(), 12);

  value.parse(arena, "-9000"_view);
  EXPECT_EQ(value.get_number(), -9000);

  value.parse(arena, "12.06"_view);
  EXPECT_EQ(value.get_real(), 12.06);

  value.parse(arena, "-130.4"_view);
  EXPECT_EQ(value.get_real(), -130.4);

  value.parse(arena, "\"Sub View\""_view);
  EXPECT_TEXT(value.get_string(), "Sub View"_view);

  value.parse(arena, "\"\""_view);
  EXPECT_TEXT(value.get_string(), ""_view);
}

PERIMORTEM_UNIT_TEST(SerializationJson, string_boundaries) {
  Allocator::Arena arena;
  Json::Node parsed;
  parsed.parse(
      arena,
      "{\"text\":\"state label : Text = \\\"Icon\\\";\",\"next\":1}"_view);

  ASSERT(parsed.is_object());
  EXPECT_NOT(parsed["text"_view].get_string().is_empty());
  EXPECT_EQ(parsed["next"_view].get_number(), 1);

  Json::Node source("line\n\"title\"\\end"_view);
  EXPECT_TEXT(source.format(arena), "\"line\\n\\\"title\\\"\\\\end\""_view);
}

PERIMORTEM_UNIT_TEST(SerializationJson, string_escapes) {
  Allocator::Arena arena;
  Json::Node value;
  value.parse(
      arena,
      "\"quote: \\\" slash: \\\\ solidus: \\/ controls: "
      "\\b\\f\\n\\r\\t unicode: \\u0041\\u03A9\\uD83D\\uDE00\""_view);

  EXPECT_TEXT(
      value.decode_string(arena),
      "quote: \" slash: \\ solidus: / controls: \b\f\n\r\t unicode: "
      "AΩ😀"_view);

  Json::Node malformed("kept \\uD800 and \\q"_view);
  EXPECT_TEXT(malformed.decode_string(arena), "kept \\uD800 and \\q"_view);

  Json::Node not_a_string(S64(42));
  EXPECT_TEXT(not_a_string.decode_string(arena), View::Bytes());
}

PERIMORTEM_UNIT_TEST(SerializationJson, parse_arrays) {
  Allocator::Arena arena;
  Json::Node value;

  value.parse(arena, "[]"_view);
  ASSERT(value.is_array());
  EXPECT_EQ(value.get_size(), 0);

  value.parse(arena, "[1,2,3,4]"_view);
  ASSERT(value.is_array());
  EXPECT_EQ(value.get_size(), 4);
  EXPECT_EQ(value[0].get_number(), 1);
  EXPECT_EQ(value[1].get_number(), 2);
  EXPECT_EQ(value[2].get_number(), 3);
  EXPECT_EQ(value[3].get_number(), 4);

  value.parse(arena, "[1,true,\"test\",4]"_view);
  ASSERT(value.is_array());
  EXPECT_EQ(value.get_size(), 4);
  EXPECT_EQ(value[0].get_number(), 1);
  EXPECT(value[1].get_flag());
  EXPECT_TEXT(value[2].get_string(), "test"_view);
  EXPECT_EQ(value[3].get_number(), 4);
}

PERIMORTEM_UNIT_TEST(SerializationJson, parse_nested_arrays) {
  Allocator::Arena arena;
  Json::Node value;

  value.parse(arena, "[]"_view);
  ASSERT(value.is_array());
  EXPECT_EQ(value.get_size(), 0);

  value.parse(arena, "[1,[2],[[true],3,[\"value\"]],[[],[[4],[]]]]"_view);
  ASSERT(value.is_array());
  EXPECT_EQ(value.get_size(), 4);
  EXPECT_EQ(value[0].get_number(), 1);

  EXPECT_EQ(value[1].get_size(), 1);
  EXPECT_EQ(value[1][0].get_number(), 2);

  EXPECT_EQ(value[2].get_size(), 3);
  EXPECT_EQ(value[2][0].get_size(), 1);
  EXPECT_EQ(value[2][1].get_size(), 0);
  EXPECT_EQ(value[2][2].get_size(), 1);
  EXPECT(value[2][0][0].get_flag());
  EXPECT_EQ(value[2][1].get_number(), 3);
  EXPECT_TEXT(value[2][2][0].get_string(), "value"_view);

  EXPECT_EQ(value[3][1][0][0].get_number(), 4);
}

PERIMORTEM_UNIT_TEST(SerializationJson, parse_object) {
  Allocator::Arena arena;
  Json::Node value;

  value.parse(arena, "{}"_view);
  ASSERT(value.is_object());
  EXPECT_EQ(value.get_size(), 0);

  value.parse(
      arena, "{\"value\":1,\"test\":2,\"another member\":3,\"final\":4}"_view);
  ASSERT(value.is_object());
  EXPECT_EQ(value.get_size(), 4);
  EXPECT_EQ(value["value"_view].get_number(), 1);
  EXPECT_EQ(value["test"_view].get_number(), 2);
  EXPECT_EQ(value["another member"_view].get_number(), 3);
  EXPECT_EQ(value["final"_view].get_number(), 4);
}

PERIMORTEM_UNIT_TEST(SerializationJson, parse_null) {
  Allocator::Arena arena;
  Json::Node value;

  // A root null literal parses to a null node.
  value.parse(arena, "null"_view);
  EXPECT(value.is_null());

  // null inside an object — the member is visible and is_null().
  value.parse(arena, "{\"key\":null}"_view);
  ASSERT(value.is_object());
  EXPECT_EQ(value.get_size(), 1);
  EXPECT(value["key"_view].is_null());

  // null alongside other members — both sides survive.
  value.parse(arena, "{\"a\":1,\"b\":null,\"c\":3}"_view);
  ASSERT(value.is_object());
  EXPECT_EQ(value.get_size(), 3);
  EXPECT_EQ(value["a"_view].get_number(), 1);
  EXPECT(value["b"_view].is_null());
  EXPECT_EQ(value["c"_view].get_number(), 3);

  // null inside an array — element is reachable and is_null().
  value.parse(arena, "[1,null,3]"_view);
  ASSERT(value.is_array());
  EXPECT_EQ(value.get_size(), 3);
  EXPECT_EQ(value[0].get_number(), 1);
  EXPECT(value[1].is_null());
  EXPECT_EQ(value[2].get_number(), 3);
}

PERIMORTEM_UNIT_TEST(SerializationJson, construct_string) {
  constexpr auto expected = "\"Test String\""_view;

  Allocator::Arena arena;
  Json::Node value = Json::Blueprint{
    {
      "Test String"_view,
    }}.construct(arena);

  auto formated = value.format(arena);
  ASSERT_TEXT(formated, expected);
}

PERIMORTEM_UNIT_TEST(SerializationJson, construct_array) {
  constexpr auto expected = "[1,2,3,4]"_view;
  const Json::Blueprint elements[] = {1, 2, 3, 4};

  Allocator::Arena arena;
  Json::Node value = Json::Blueprint(elements).construct(arena);

  auto formated = value.format(arena);
  ASSERT_TEXT(formated, expected);
}

PERIMORTEM_UNIT_TEST(SerializationJson, construct_nested) {
  constexpr auto expected =
      "{\"root\":[1,2,true,false,\"Test\"],\"flag\":true,\"config\":{"
      "\"version\":\"1.0.2\",\"sub_flag\":true}}"_view;

  Allocator::Arena arena;
  Json::Node value = Json::Blueprint{
    {
      {"root"_view,
       {
         1,
         2,
         True,
         False,
         "Test"_view,
       }},
      {"flag"_view, True},
      {"config"_view,
       {
         {"version"_view, "1.0.2"_view},
         {"sub_flag"_view, True},
       }},
    }}.construct(arena);

  auto formated = value.format(arena);
  ASSERT_TEXT(formated, expected);
}

PERIMORTEM_UNIT_TEST(SerializationJson, round_trip_init_rpc) {
  auto source = File::read("validation/data/json/init_rpc.json"_view);
  ASSERT(source);
  ASSERT_NOT((*source).is_empty());

  Allocator::Arena arena;
  Json::Node value;
  value.parse(arena, *source);
  auto formated = value.format(arena);

  ASSERT_EQ(formated.get_size(), (*source).get_size());
  ASSERT_TEXT(formated, (*source).get_view());
}

PERIMORTEM_UNIT_TEST(SerializationJson, format_number_zero) {
  Allocator::Arena arena;
  auto value = Json::Blueprint{
    {
      {"a"_view, 0},
      {"b"_view, 1},
      {"c"_view, 10},
    }}.construct(arena);

  constexpr auto expected = "{\"a\":0,\"b\":1,\"c\":10}"_view;
  auto formated = value.format(arena);
  ASSERT_TEXT(formated, expected);
}

PERIMORTEM_UNIT_TEST(SerializationJson, empty_array) {
  Allocator::Arena arena;
  auto value = Json::Blueprint{
    {
      {"name"_view, "items"_view},
      Json::Blueprint::empty_array("items"_view),
    }}.construct(arena);

  constexpr auto expected = "{\"name\":\"items\",\"items\":[]}"_view;
  auto formated = value.format(arena);
  ASSERT_TEXT(formated, expected);
}

PERIMORTEM_UNIT_TEST(SerializationJson, existing_node) {
  Allocator::Arena arena;

  const Json::Node inner_node = Json::Blueprint{
    {
      {"name"_view, "ttx-server"_view},
      {"version"_view, "1.0"_view},
    }}.construct(arena);

  auto value = Json::Blueprint{
    {
      {"jsonrpc"_view, "2.0"_view},
      {"id"_view, 1},
      {"result"_view, inner_node},
    }}.construct(arena);

  constexpr auto expected =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"name\":\"ttx-server\","
      "\"version\":\"1.0\"}}"_view;
  auto formated = value.format(arena);
  ASSERT_TEXT(formated, expected);
}

PERIMORTEM_UNIT_TEST(SerializationJson, format_null) {
  Allocator::Arena arena;
  Json::Node value;

  // A root null serializes to "null".
  value.parse(arena, "null"_view);
  ASSERT_TEXT(value.format(arena), "null"_view);

  // An object with a null member reproduces its exact text.
  value.parse(arena, "{\"a\":1,\"b\":null,\"c\":3}"_view);
  ASSERT_TEXT(value.format(arena), "{\"a\":1,\"b\":null,\"c\":3}"_view);
}

PERIMORTEM_UNIT_TEST(SerializationJson, rpc_from_parsed) {
  auto source = File::read("validation/data/json/init_rpc.json"_view);
  ASSERT(source);
  ASSERT_NOT((*source).is_empty());

  Allocator::Arena arena;
  Json::Node parsed;
  parsed.parse(arena, *source);

  ASSERT(parsed["jsonrpc"_view].is_string());
  ASSERT(parsed["id"_view].is_number());

  auto response = Json::Blueprint{
    {
      {"jsonrpc"_view, parsed["jsonrpc"_view].get_string()},
      {"id"_view, parsed["id"_view].get_number()},
      {"result"_view,
       {
         {"serverInfo"_view, "ttx"_view},
       }},
    }}.construct(arena);
  auto formated = response.format(arena);

  ASSERT(formated.get_size() > 0);
  ASSERT(response.is_object());
  EXPECT_TEXT(response["jsonrpc"_view].get_string(), "2.0"_view);
  EXPECT_EQ(response["id"_view].get_number(), 10);
}
