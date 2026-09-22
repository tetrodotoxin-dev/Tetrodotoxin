// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/utility/result.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Validation;

static Harness UtilityResult = {
  .name = "Utility::Result"_view,
};

enum class ResultError : U8 {
  Unknown = U8(-1),
  Rejected = 0,
};

class OwnedResultValue {
 public:
  OwnedResultValue(S32 value, Count& live_values)
      : value(value), live_values(live_values) {
    live_values++;
  }

  OwnedResultValue(const OwnedResultValue& source)
      : value(source.value), live_values(source.live_values) {
    live_values++;
  }

  OwnedResultValue(OwnedResultValue&& source)
      : value(source.value), live_values(source.live_values) {
    live_values++;
    source.value = 0;
  }

  ~OwnedResultValue() { live_values--; }

  auto increment() -> void { value++; }
  auto get() const -> S32 { return value; }

 private:
  S32 value;
  Count& live_values;
};

class OwnedResultError {
 public:
  ~OwnedResultError() {}
};

class MoveOnlyResultValue {
 public:
  MoveOnlyResultValue() {}
  MoveOnlyResultValue(const MoveOnlyResultValue&) = delete;
  MoveOnlyResultValue(MoveOnlyResultValue&&) {}
  ~MoveOnlyResultValue() {}
};

template <typename error_type>
concept SupportedResultError = requires { typename Result<S32, error_type>; };

static auto create_owned_value(Count& live_values)
    -> Result<OwnedResultValue, ResultError> {
  OwnedResultValue value(41, live_values);
  return Data::take(value);
}

PERIMORTEM_UNIT_TEST(UtilityResult, visits_value) {
  Result<S32, ResultError> selected(S32(41));

  selected.visit([](S32& value) -> void { value++; }, [](ResultError) {});

  const auto& observed = selected;
  S32 value = observed.visit(
      [](S32 selected) { return selected; },
      [](ResultError) { return S32(0); });
  EXPECT_EQ(value, S32(42));
}

PERIMORTEM_UNIT_TEST(UtilityResult, visits_error) {
  Result<S32, ResultError> selected(ResultError::Rejected);
  Result<S32, ResultError> copied(selected);
  Result<S32, ResultError> moved(Data::take(copied));

  ResultError error = moved.visit(
      [](S32) { return ResultError::Unknown; },
      [](ResultError selected) { return selected; });

  EXPECT(error == ResultError::Rejected);
}

PERIMORTEM_UNIT_TEST(UtilityResult, preserves_reference) {
  S32 value = 41;
  Result<S32&, ResultError> selected(value);

  selected.visit([](S32& selected) -> void { selected++; }, [](ResultError) {});

  EXPECT_EQ(value, S32(42));
}

PERIMORTEM_UNIT_TEST(UtilityResult, owns_dynamic_value) {
  Result<Dynamic::Bytes, ResultError> selected(Dynamic::Bytes("owned"_view));

  selected.visit(
      [](Dynamic::Bytes& value) -> void { value.concat(" value"_view); },
      [](ResultError) {});

  const auto& observed = selected;
  View::Bytes value = observed.visit(
      [](const Dynamic::Bytes& selected) { return selected.get_view(); },
      [](ResultError) { return View::Bytes(); });
  EXPECT_TEXT(value, "owned value"_view);
}

PERIMORTEM_UNIT_TEST(UtilityResult, owns_lifetime) {
  Count live_values = 0;

  {
    auto selected = create_owned_value(live_values);
    EXPECT_EQ(live_values, Count(1));

    selected.visit(
        [](OwnedResultValue& value) -> void { value.increment(); },
        [](ResultError) {});

    const auto& observed = selected;
    S32 value = observed.visit(
        [](const OwnedResultValue& selected) { return selected.get(); },
        [](ResultError) { return S32(0); });
    EXPECT_EQ(value, S32(42));
  }

  EXPECT_EQ(live_values, Count(0));
}

PERIMORTEM_UNIT_TEST(UtilityResult, copies_and_moves) {
  Count live_values = 0;

  {
    auto first = create_owned_value(live_values);
    Result<OwnedResultValue, ResultError> copied(first);
    Result<OwnedResultValue, ResultError> moved(Data::take(copied));
    EXPECT_EQ(live_values, Count(3));

    moved.visit(
        [](OwnedResultValue& value) -> void { value.increment(); },
        [](ResultError) {});

    S32 first_value = first.visit(
        [](const OwnedResultValue& selected) { return selected.get(); },
        [](ResultError) { return S32(0); });
    S32 moved_value = moved.visit(
        [](const OwnedResultValue& selected) { return selected.get(); },
        [](ResultError) { return S32(0); });
    EXPECT_EQ(first_value, S32(41));
    EXPECT_EQ(moved_value, S32(42));
  }

  EXPECT_EQ(live_values, Count(0));
}

PERIMORTEM_UNIT_TEST(UtilityResult, switches_state) {
  Count live_values = 0;

  {
    auto selected = create_owned_value(live_values);
    EXPECT_EQ(live_values, Count(1));

    selected = ResultError::Rejected;
    EXPECT_EQ(live_values, Count(0));
    EXPECT(selected.visit(
        [](const OwnedResultValue&) { return False; },
        [](ResultError error) {
          return error == ResultError::Rejected ? True : False;
        }));

    selected = create_owned_value(live_values);
    EXPECT_EQ(live_values, Count(1));
    EXPECT(selected.visit(
        [](const OwnedResultValue& value) {
          return value.get() == S32(41) ? True : False;
        },
        [](ResultError) { return False; }));

    auto copied = create_owned_value(live_values);
    selected = copied;
    EXPECT_EQ(live_values, Count(2));
    EXPECT(selected.visit(
        [](const OwnedResultValue& value) {
          return value.get() == S32(41) ? True : False;
        },
        [](ResultError) { return False; }));
  }

  EXPECT_EQ(live_values, Count(0));
}

static_assert(!__is_constructible(Result<S32, ResultError>));
static_assert(__is_constructible(Result<S32, ResultError>, S32));
static_assert(__is_constructible(Result<S32, ResultError>, ResultError));
static_assert(!__is_constructible(Result<S32&, ResultError>, S32&&));
static_assert(!__is_trivially_destructible(OwnedResultValue));
static_assert(__is_trivially_destructible(Result<S32, ResultError>));
static_assert(__is_constructible(
    Result<OwnedResultValue, ResultError>,
    OwnedResultValue&&));
static_assert(__is_constructible(
    Result<MoveOnlyResultValue, ResultError>,
    MoveOnlyResultValue&&));
static_assert(!__is_constructible(
    Result<MoveOnlyResultValue, ResultError>,
    const Result<MoveOnlyResultValue, ResultError>&));
static_assert(!SupportedResultError<OwnedResultError>);
