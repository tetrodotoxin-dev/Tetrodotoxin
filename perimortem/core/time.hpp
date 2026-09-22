// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Core {

// Time captures a relative timestamp that can be used to compare time points.
class Time {
 public:
  // Duration captures the explicit distance between any two time points.
  // Allows for centralized conversion into actual time units for measurement.
  class Duration {
   public:
    Duration(U64 delta) : nanosecond_delta(delta) {};

    // Converts a delta time into nanoseconds.
    constexpr auto convert_to_nanoseconds() -> U64 { return nanosecond_delta; }

    // Converts a delta time into microseconds.
    constexpr auto convert_to_microseconds() -> R64 {
      return nanosecond_delta / R64(1'000.0);
    }

    // Converts a delta time into milliseconds.
    constexpr auto convert_to_milliseconds() -> R64 {
      return nanosecond_delta / R64(1'000'000.0);
    }

    // Converts a delta time into seconds.
    // Drops the micro & nano second portions.
    constexpr auto convert_to_seconds() -> R64 {
      return (nanosecond_delta / 1'000'000.0) / R64(1'000.0);
    }

   private:
    U64 nanosecond_delta;
  };

  Time() : timestamp(0) {};
  Time(U64 stamp) : timestamp(stamp) {};
  Time(U64 seconds, U64 nanoseconds)
      : timestamp(seconds * 1'000'000'000 + nanoseconds) {};

  // Returns a time object capturing the delta time from UNIX epoch.
  static auto now() -> Time;

  // Returns the best guess at wall clock time.
  static auto clock() -> Time;

  // Returns the time the application booted.
  static auto boot() -> Time;

  // Returns an arbitrary time point that should never exist.
  static auto never() -> Time { return -1; };

  // Checks if two deltas are equal to each other, but doesn't guarantee they
  // are in the same reference frame.
  constexpr auto operator==(const Time& rhs) const -> Bool {
    return get_stamp() == rhs.get_stamp();
  };

  // Gets the raw 64 bit time stamp in nanoseconds.
  constexpr auto get_stamp() const -> U64 { return timestamp; }

  // Calculation the time between two time points.
  // If none is provided then `Time::now()` is used as the end point.
  constexpr auto measure(const Time& end_point = Time::now()) const
      -> Duration {
    // Ensure duration is always the absolute value.
    auto start_stamp = Math::min(get_stamp(), end_point.get_stamp());
    auto end_stamp = Math::max(get_stamp(), end_point.get_stamp());
    return Duration(end_stamp - start_stamp);
  }

  // Returns the wall clock time as a formatted buffer.
  auto calculate_clock() const -> Static::Bytes<12>;

 private:
  U64 timestamp;
};

}  // namespace Perimortem::Core
