// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/args.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Validation;

static Harness SystemArgs = {
  .name = "System::Args"_view,
  .setup =
      []() {
        Diagnostics::Log::set_sink(Test::capture_sink);
        Diagnostics::Log::set_level(Diagnostics::Log::Level::Info);
      },
  .teardown =
      []() {
        Diagnostics::Log::set_sink(Diagnostics::Log::default_sink);
        Diagnostics::Log::set_level(Diagnostics::Log::Level::Info);
      },
};

using Configs = Managed::Map<View::Bytes, View::Bytes>;

static auto test_config(Allocator::Arena& arena) -> Configs {
  Configs variables(arena);
  variables.insert("fast"_view, "Enable fast path."_view);
  variables.insert("output"_view, "Output file."_view);
  variables.insert("dep"_view, "Dependency file."_view);
  variables.insert("threads"_view, "Worker count."_view);
  variables.insert("ratio"_view, "Ratio value."_view);
  return variables;
}

static auto parse(Allocator::Arena& arena, View::Vector<View::Bytes> arguments)
    -> Args::Values {
  return Args::parse(arena, test_config(arena), arguments);
}

static auto values_for(const Args::Values& args, View::Bytes name)
    -> View::Vector<View::Bytes> {
  auto entry = args.find(name);
  if (!entry) {
    return View::Vector<View::Bytes>();
  }

  return (*entry).value->get_view();
}

static auto value_count(const Args::Values& args, View::Bytes name) -> Count {
  return values_for(args, name).get_size();
}

static auto value_at(
    const Args::Values& args,
    View::Bytes name,
    Count index = 0) -> View::Bytes {
  View::Vector<View::Bytes> values = values_for(args, name);
  if (index >= values.get_size()) {
    return View::Bytes();
  }

  return values.get_data()[index];
}

static constexpr View::Bytes expected_help =
    "usage: demo [arguments]\n\n"
    "Test parser.\n\n"
    "arguments:\n"
    "  -dep      Dependency file.\n"
    "  -threads  Worker count.\n"
    "  -output   Output file.\n"
    "  -ratio    Ratio value.\n"
    "  -fast     Enable fast path.\n"
    "  -help     Show this help.\n"_view;

PERIMORTEM_UNIT_TEST(SystemArgs, basic_parse) {
  constexpr Static::Vector<View::Bytes, 6> raw = {
    {"demo"_view, "-fast"_view, "-output=out.a"_view, "-dep=dep.ttx"_view,
     "-threads=-8"_view, "-ratio=0.25"_view}};
  Allocator::Arena arena;

  Args::Values parsed = parse(arena, raw);
  ASSERT_NOT(parsed.is_empty());
  EXPECT(parsed.contains("fast"_view));
  EXPECT_EQ(value_count(parsed, "fast"_view), Count(1));
  EXPECT_TEXT(value_at(parsed, "fast"_view), "true"_view);
  EXPECT_TEXT(value_at(parsed, "threads"_view), "-8"_view);
  EXPECT_TEXT(value_at(parsed, "ratio"_view), "0.25"_view);
  EXPECT_TEXT(value_at(parsed, "output"_view), "out.a"_view);
  EXPECT_TEXT(value_at(parsed, "dep"_view), "dep.ttx"_view);
}

PERIMORTEM_UNIT_TEST(SystemArgs, help_text) {
  constexpr Static::Vector<View::Bytes, 2> raw = {
    {"/tmp/demo"_view, "-help"_view}};
  Allocator::Arena arena;
  Configs config = test_config(arena);

  Args::Values parsed = Args::parse(arena, config, raw);
  ASSERT(parsed.contains("help"_view));
  EXPECT_TEXT(value_at(parsed, "help"_view), "true"_view);
  Args::log_help(arena, "Test parser."_view, config, raw);
  EXPECT_TEXT(Test::captured_message(), expected_help);
}

PERIMORTEM_UNIT_TEST(SystemArgs, double_dash_help) {
  constexpr Static::Vector<View::Bytes, 2> raw = {{"demo"_view, "--help"_view}};
  Allocator::Arena arena;
  Configs config = test_config(arena);

  Args::Values parsed = Args::parse(arena, config, raw);
  ASSERT(parsed.contains("help"_view));
  EXPECT_TEXT(value_at(parsed, "help"_view), "true"_view);
  Args::log_help(arena, "Test parser."_view, config, raw);
  EXPECT_TEXT(Test::captured_message(), expected_help);
}

PERIMORTEM_UNIT_TEST(SystemArgs, help_keeps_parsing) {
  constexpr Static::Vector<View::Bytes, 4> raw = {
    {"demo"_view, "-help"_view, "-fast"_view, "-output=out.a"_view}};
  Allocator::Arena arena;

  Args::Values parsed = parse(arena, raw);
  ASSERT(parsed.contains("help"_view));
  EXPECT(parsed.contains("fast"_view));
  EXPECT_TEXT(value_at(parsed, "help"_view), "true"_view);
  EXPECT_TEXT(value_at(parsed, "fast"_view), "true"_view);
  EXPECT_TEXT(value_at(parsed, "output"_view), "out.a"_view);
}

PERIMORTEM_UNIT_TEST(SystemArgs, empty_value) {
  constexpr Static::Vector<View::Bytes, 2> raw = {
    {"demo"_view, "-output="_view}};
  Allocator::Arena arena;

  Args::Values parsed = parse(arena, raw);
  ASSERT_NOT(parsed.is_empty());
  EXPECT_TEXT(value_at(parsed, "output"_view), ""_view);
}

PERIMORTEM_UNIT_TEST(SystemArgs, unknown_arg) {
  constexpr Static::Vector<View::Bytes, 3> raw = {
    {"demo"_view, "-other"_view, "-output=out"_view}};
  Allocator::Arena arena;

  Diagnostics::Log::set_level(Diagnostics::Log::Level::Error);
  Args::Values parsed = parse(arena, raw);
  ASSERT(parsed.is_empty());
  EXPECT(Test::error_contains("unrecognized arg -other"_view));
}

PERIMORTEM_UNIT_TEST(SystemArgs, bare_value) {
  constexpr Static::Vector<View::Bytes, 3> raw = {
    {"demo"_view, "-output=out"_view, "source.ttx"_view}};
  Allocator::Arena arena;

  Diagnostics::Log::set_level(Diagnostics::Log::Level::Error);
  Args::Values parsed = parse(arena, raw);
  ASSERT(parsed.is_empty());
  EXPECT(Test::error_contains("unrecognized arg source.ttx"_view));
}

PERIMORTEM_UNIT_TEST(SystemArgs, repeated_values) {
  constexpr Static::Vector<View::Bytes, 4> raw = {
    {"demo"_view, "-output=out"_view, "-dep=a.ttx"_view, "-dep=b.ttx"_view}};
  Allocator::Arena arena;

  Args::Values parsed = parse(arena, raw);
  ASSERT_NOT(parsed.is_empty());
  ASSERT_EQ(value_count(parsed, "dep"_view), Count(2));
  EXPECT_TEXT(value_at(parsed, "dep"_view, 0), "a.ttx"_view);
  EXPECT_TEXT(value_at(parsed, "dep"_view, 1), "b.ttx"_view);
}

PERIMORTEM_UNIT_TEST(SystemArgs, dash_only) {
  constexpr Static::Vector<View::Bytes, 2> raw = {{"demo"_view, "--"_view}};
  Allocator::Arena arena;

  Diagnostics::Log::set_level(Diagnostics::Log::Level::Error);
  Args::Values parsed = parse(arena, raw);
  ASSERT(parsed.is_empty());
  EXPECT(Test::error_contains("unrecognized arg --"_view));
}
