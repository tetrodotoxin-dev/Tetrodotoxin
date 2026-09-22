// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Validation;

constexpr Count max_message_length = 256;

struct LogEvent {
  Static::Bytes<max_message_length> message;
  Count message_size;
};

constexpr Count event_log_size = 8;
static Static::Vector<LogEvent, event_log_size> log_events;
static Count total_events = 0;

static auto capture_sink(
    Diagnostics::Log::Level level,
    View::Bytes message,
    const Diagnostics::Source& location) -> void {
  Static::Bytes<max_message_length> formatted;
  Count formatted_size = Diagnostics::Log::format_entry(
      level, message, location, formatted.get_access());
  Count index = total_events++ % event_log_size;
  log_events[index].message_size =
      Math::min(max_message_length, formatted_size);
  log_events[index].message = formatted;
}

static auto last_entry() -> View::Bytes {
  auto& event = log_events[(total_events - 1) % event_log_size];
  return View::Bytes(event.message.get_data(), event.message_size);
}

static auto contains(View::Bytes haystack, View::Bytes message) -> Bool {
  return Algorithm::search(haystack, message) != Count(-1);
}

static auto has_valid_header(View::Bytes entry) -> Bool {
  constexpr auto header_length = 14;
  if (entry.get_size() < header_length) {
    return false;
  }

  // Header bytes
  switch (entry[0]) {
  case 'D':
  case 'I':
  case 'W':
  case 'E':
  case 'F':
    break;
  default:
    return false;
  }

  // Validate all number values
  const U8* b = entry.get_data() + 2;
  if (b[2] != ':' || b[5] != ':' || b[8] != '.') {
    return false;
  }

  // Validate all number values
  constexpr Static::Vector<Count, 9> number_indexes = {{
    0,
    1,
    3,
    4,
    6,
    7,
    9,
    10,
    11,
  }};
  for (Count i = 0; i < number_indexes.get_size(); i++) {
    if (b[number_indexes[i]] < '0' || b[number_indexes[i]] > '9') {
      return false;
    }
  }

  return true;
}

static Harness DiagnosticsLog = {
  .name = "Diagnostics::Log"_view,
  .setup =
      []() {
        Diagnostics::Log::set_sink(capture_sink);
        Diagnostics::Log::set_level(Diagnostics::Log::Level::Debug);
        Diagnostics::Log::set_disable_header(False);
        total_events = 0;
      },
  .teardown =
      []() {
        Diagnostics::Log::set_sink(Diagnostics::Log::default_sink);
        Diagnostics::Log::set_level(Diagnostics::Log::Level::Info);
        Diagnostics::Log::set_disable_header(False);
        total_events = 0;
      },
};

PERIMORTEM_UNIT_TEST(DiagnosticsLog, info_record) {
  Diagnostics::Log::info("unique message string"_view);
  View::Bytes entry = last_entry();

  ASSERT(entry.get_size() > 0);
  EXPECT(has_valid_header(entry));
  EXPECT_EQ(entry[entry.get_size() - 1], U8('\n'));
  EXPECT(
      contains(entry, "validation/unit_tests/perimortem/core/log.cpp:"_view));
  EXPECT(contains(entry, "[main]"_view));
  EXPECT(contains(entry, "unique message string"_view));
}

PERIMORTEM_UNIT_TEST(DiagnosticsLog, message_raii_guard) {
  Count events_before = total_events;

  {
    Diagnostics::Log::Message<64> message(Diagnostics::Log::Level::Info);
    message << "builder emitted value="_view << U32(42);
    EXPECT_EQ(total_events, events_before);
  }

  EXPECT(total_events > events_before);
  EXPECT(contains(last_entry(), "builder emitted value=42"_view));
  EXPECT(has_valid_header(last_entry()));
}

PERIMORTEM_UNIT_TEST(DiagnosticsLog, suppress_messages) {
  Diagnostics::Log::set_level(Diagnostics::Log::Level::Error);
  Count events_before = total_events;

  Diagnostics::Log::error("should pass through"_view);
  EXPECT(total_events > events_before);
  events_before = total_events;

  Diagnostics::Log::info("should be suppressed"_view);
  EXPECT_EQ(total_events, events_before);

  // Surpressed message shouldn't override the older message.
  EXPECT(contains(last_entry(), "should pass through"_view));
  EXPECT(has_valid_header(last_entry()));
}

static auto logging_function() -> void {
  Diagnostics::Log::error("Test Attribution"_view);
}

static auto attributing_function() -> void {
  auto scope_attribution = Diagnostics::Log::set_attribution();
  logging_function();
}

PERIMORTEM_UNIT_TEST(DiagnosticsLog, attribution) {
  attributing_function();

  auto logged = last_entry();
  EXPECT(contains(
      logged, "[main] validation/unit_tests/perimortem/core/log.cpp:"_view));
  EXPECT(contains(logged, "Test Attribution"_view));
  EXPECT(has_valid_header(last_entry()));
}
