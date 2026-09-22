// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// Simplified test framework for Perimortem. Now that validation links directly
// against Perimortem, the framework uses Perimortem types throughout rather
// than raw C equivalents.
//
// One runner owns registration and reporting while each test translation unit
// contributes only its cases and Harness.

#pragma once

#include "validation/harness.hpp"

#include "perimortem/core/diagnostics/log.hpp"

namespace Validation::Test {

auto capture_sink(
    Perimortem::Core::Diagnostics::Log::Level level,
    Perimortem::Core::View::Bytes message,
    const Perimortem::Core::Diagnostics::Source& location) -> void;
auto captured_message() -> Perimortem::Core::View::Bytes;
auto error_contains(
    Perimortem::Core::View::Bytes message,
    Perimortem::Core::Diagnostics::Log::Level level =
        Perimortem::Core::Diagnostics::Log::Level::Error) -> Bool;

auto expected(Bool value, Bool actual) -> void;
auto expected(Perimortem::Core::View::Bytes value, Bool actual) -> void;
auto expected(S16 value, Bool actual) -> void;
auto expected(U16 value, Bool actual) -> void;
auto expected(U32 value, Bool actual) -> void;
auto expected(U64 value, Bool actual) -> void;
auto expected(S32 value, Bool actual) -> void;
auto expected(S64 value, Bool actual) -> void;
auto expected(CppSize value, Bool actual) -> void;
auto expected(R64 value, Bool actual) -> void;
auto expected_text(
    Perimortem::Core::View::Bytes value,
    Perimortem::Core::View::Bytes other,
    Bool actual) -> void;
auto expected_hex(Perimortem::Core::View::Bytes value, Bool actual) -> void;

enum class TestResult {
  Pass,
  Failed,
};

using TestFunc = void (*)(TestResult& result);

auto log_message(
    Perimortem::Core::View::Bytes file,
    Count line,
    Perimortem::Core::View::Bytes message) -> void;

auto create(
    const Harness& harness,
    Perimortem::Core::View::Bytes name,
    TestFunc func,
    Perimortem::Core::View::Bytes file,
    Count line) -> void;

#define PRINT_RESULT()                          \
  {                                             \
    Validation::Test::expected(__check, false); \
    Validation::Test::expected(__value, true);  \
  }

#define PRINT_TEXT()                                          \
  {                                                           \
    Validation::Test::expected_text(__check, __value, false); \
    Validation::Test::expected_text(__value, __check, true);  \
  }

#define PRINT_HEX()                                 \
  {                                                 \
    Validation::Test::expected_hex(__check, false); \
    Validation::Test::expected_hex(__value, true);  \
  }

#define EXPECT(expression)                                               \
  {                                                                      \
    auto __value = bool(expression);                                     \
    auto __check = true;                                                 \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " failed test"));                              \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
    }                                                                    \
  }

#define ASSERT(expression)                                               \
  {                                                                      \
    auto __value = bool(expression);                                     \
    auto __check = true;                                                 \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " failed test"));                              \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
      return;                                                            \
    }                                                                    \
  }

#define EXPECT_NOT(expression)                                           \
  {                                                                      \
    auto __value = bool(expression);                                     \
    auto __check = false;                                                \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " should be false"));                          \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
    }                                                                    \
  }

#define ASSERT_NOT(expression)                                           \
  {                                                                      \
    auto __value = bool(expression);                                     \
    auto __check = false;                                                \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " should be false"));                          \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
      return;                                                            \
    }                                                                    \
  }

#define EXPECT_EQ(expression, expect)                                    \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " != " #expect));                              \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
    }                                                                    \
  }

#define EXPECT_NEQ(expression, expect)                                   \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (__value == __check) {                                            \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " == " #expect));                              \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
    }                                                                    \
  }

#define ASSERT_EQ(expression, expect)                                    \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " != " #expect));                              \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
      return;                                                            \
    }                                                                    \
  }

#define ASSERT_NEQ(expression, expect)                                   \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (__value == __check) {                                            \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " == " #expect));                              \
      PRINT_RESULT();                                                    \
      result = Validation::Test::TestResult::Failed;                     \
      return;                                                            \
    }                                                                    \
  }

#define EXPECT_TEXT(expression, expect)                                  \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " != " #expect));                              \
      PRINT_TEXT();                                                      \
      result = Validation::Test::TestResult::Failed;                     \
    }                                                                    \
  }

#define ASSERT_TEXT(expression, expect)                                  \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " != " #expect));                              \
      PRINT_TEXT();                                                      \
      result = Validation::Test::TestResult::Failed;                     \
      return;                                                            \
    }                                                                    \
  }

#define EXPECT_HEX(expression, expect)                                   \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " != " #expect));                              \
      PRINT_HEX();                                                       \
      result = Validation::Test::TestResult::Failed;                     \
    }                                                                    \
  }

#define ASSERT_HEX(expression, expect)                                   \
  {                                                                      \
    auto __value = (expression);                                         \
    auto __check = (expect);                                             \
    if (!(__value == __check)) {                                         \
      Validation::Test::log_message(                                     \
          Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__, \
          Perimortem::Core::NullTerminated::to_view(                     \
              #expression " != " #expect));                              \
      PRINT_HEX();                                                       \
      result = Validation::Test::TestResult::Failed;                     \
      return;                                                            \
    }                                                                    \
  }

class TestEntry {
 public:
  TestEntry(
      const Harness& harness,
      Perimortem::Core::View::Bytes name,
      TestFunc func,
      Perimortem::Core::View::Bytes file,
      Count line) {
    create(harness, name, func, file, line);
  }
};

}  // namespace Validation::Test

#define PERIMORTEM_UNIT_TEST(harness, name)                                  \
  static auto validation_test_##harness##_##name(                            \
      Validation::Test::TestResult& result) -> void;                         \
  namespace {                                                                \
  Validation::Test::TestEntry validation_registration_##harness##_##name = { \
    harness, #name##_view, validation_test_##harness##_##name,               \
    Perimortem::Core::NullTerminated::to_view(__FILE__), __LINE__};          \
  }                                                                          \
  static auto validation_test_##harness##_##name(                            \
      Validation::Test::TestResult& result) -> void
