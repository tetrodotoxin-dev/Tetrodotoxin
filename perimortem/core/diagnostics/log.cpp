// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/diagnostics/log.hpp"

#include <stdio.h>
#include <stdlib.h>

#ifdef PERI_LINUX
#include <execinfo.h>
#endif

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/thread/worker.hpp"
#include "perimortem/core/time.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;

constexpr Count max_message_capacity = 1 << 11;

class ThreadWriter {
 public:
  ~ThreadWriter() {
    if (file) {
      fflush(file);
      fclose(file);
      file = nullptr;
    }
  }

  auto prepare_file() -> void {
    Static::Bytes<512> name_buffer;
    Writer::Textual writer(name_buffer.get_access().slice(0, 511));

    writer << "perimortem_"_view;
    writer << "["_view << Thread::Worker::get_thread_name() << "]_"_view;

    writer << Time::now().get_stamp();
    writer << ".log"_view;
    name_buffer[writer.get_location()] = '\0';

    file = fopen(Data::cast<char>(name_buffer.get_data()), "ab");
    file_ready = True;
  }

  auto accumulate(View::Bytes entry) -> void {
    if (!file_ready) {
      prepare_file();
    }

    if (!file) {
      return;
    }

    fwrite(entry.get_data(), 1, entry.get_size(), file);
  }

  auto flush() -> void {
    if (file) {
      fflush(file);
    }
  }

 private:
  FILE* file = nullptr;
  Bool file_ready = False;
};

static thread_local Diagnostics::Log::Sink message_sink =
    Diagnostics::Log::default_sink;
static thread_local Diagnostics::Log::Level thread_log_level =
    Diagnostics::Log::Level::Info;
static thread_local Bool disable_log_header = False;
static thread_local Diagnostics::Source attribution_override;
static thread_local ThreadWriter thread_writer;

constexpr auto level_char(Diagnostics::Log::Level level) -> char {
  switch (level) {
  case Diagnostics::Log::Level::Debug:
    return 'D';
  case Diagnostics::Log::Level::Info:
    return 'I';
  case Diagnostics::Log::Level::Warning:
    return 'W';
  case Diagnostics::Log::Level::Error:
    return 'E';
  case Diagnostics::Log::Level::Fatal:
    return 'F';
  }

  return '?';
}

constexpr auto level_color(Diagnostics::Log::Level level) -> View::Bytes {
  switch (level) {
  case Diagnostics::Log::Level::Debug:
    return "\x1b[38;5;246m"_view;
  case Diagnostics::Log::Level::Info:
    return ""_view;
  case Diagnostics::Log::Level::Warning:
    return "\x1b[38;5;220m"_view;
  case Diagnostics::Log::Level::Error:
  case Diagnostics::Log::Level::Fatal:
    return "\x1b[38;5;160m"_view;
  }

  return ""_view;
}

static auto format_message(
    Diagnostics::Log::Level level,
    View::Bytes message,
    const Diagnostics::Source& location,
    Access::Bytes output) -> Count {
  if (!disable_log_header) {
    return Diagnostics::Log::format_entry(level, message, location, output);
  }

  Writer::Textual writer(output);
  if (location.is_set()) {
    writer << location.get_file() << ':' << location.get_line() << ':'
           << location.get_column() << ": "_view;
  }

  writer << message;
  return writer.get_location();
}

Diagnostics::Log::Attribution::Attribution(Attribution&& rhs) {
  primary_guard = rhs.primary_guard;
  rhs.primary_guard = False;
}

Diagnostics::Log::Attribution::~Attribution() {
  if (primary_guard) {
    attribution_override = Source();
  }
}

auto Diagnostics::Log::file_sink(
    Level level,
    View::Bytes message,
    const Source& location) -> void {
  Static::Bytes<max_message_capacity> message_buffer;
  Count message_length =
      format_message(level, message, location, message_buffer.get_access());
  thread_writer.accumulate(message_buffer.slice(0, message_length));
}

auto Diagnostics::Log::console_sink(
    Level level,
    View::Bytes message,
    const Source& location) -> void {
  Static::Bytes<max_message_capacity> message_buffer;
  Count message_length =
      format_message(level, message, location, message_buffer.get_access());
  View::Bytes formatted = message_buffer.slice(0, message_length);
  FILE* stream = (level >= Level::Error) ? stderr : stdout;
  fwrite(formatted.get_data(), 1, formatted.get_size(), stream);
}

auto Diagnostics::Log::plain_sink(
    Level level,
    View::Bytes message,
    const Source&) -> void {
  if (message.is_empty()) {
    return;
  }

  FILE* stream = (level >= Level::Error) ? stderr : stdout;
  fwrite(message.get_data(), 1, message.get_size(), stream);
  if (message[message.get_size() - 1] != '\n') {
    constexpr char newline = '\n';
    fwrite(&newline, 1, 1, stream);
  }
}

auto Diagnostics::Log::color_sink(
    Level level,
    View::Bytes message,
    const Source& location) -> void {
  Static::Bytes<max_message_capacity> entry_buffer;
  Count entry_length =
      format_message(level, message, location, entry_buffer.get_access());
  View::Bytes entry = entry_buffer.slice(0, entry_length);
  if (message.is_empty()) {
    return;
  }

  // 16 extra bytes is enough for any color code + the clear code.
  Static::Bytes<max_message_capacity + 16> color_buffer;
  Writer::Textual writer(color_buffer.get_access());

  writer << level_color(level);
  writer << entry;
  writer << "\x1b[0m"_view;

  FILE* stream = (level >= Level::Error) ? stderr : stdout;
  fwrite(color_buffer.get_data(), 1, writer.get_location(), stream);
}

auto Diagnostics::Log::stderr_sink(
    Level level,
    View::Bytes message,
    const Source& location) -> void {
  Static::Bytes<max_message_capacity> message_buffer;
  Count message_length =
      format_message(level, message, location, message_buffer.get_access());
  View::Bytes formatted = message_buffer.slice(0, message_length);
  fwrite(formatted.get_data(), 1, formatted.get_size(), stderr);
  fflush(stderr);
}

auto Diagnostics::Log::debug_sink(
    Level level,
    View::Bytes message,
    const Source& location) -> void {
  console_sink(level, message, location);
  file_sink(level, message, location);
}

auto Diagnostics::Log::set_sink(Sink sink) -> void {
  message_sink = sink;
}

auto Diagnostics::Log::get_sink() -> Sink {
  return message_sink;
}

auto Diagnostics::Log::set_disable_header(Bool disable_header) -> void {
  disable_log_header = disable_header;
}

auto Diagnostics::Log::get_disable_header() -> Bool {
  return disable_log_header;
}

auto Diagnostics::Log::set_level(Level level) -> void {
  thread_log_level = level;
}

auto Diagnostics::Log::get_level() -> Diagnostics::Log::Level {
  return thread_log_level;
}

auto Diagnostics::Log::set_attribution(const Source& location) -> Attribution {
  // If we have someone already claiming attribution higher on the stack then
  // ignore the request.
  // If there is no attribution then create an attribution point.
  Attribution scope_guard;
  scope_guard.primary_guard = !attribution_override.is_set();
  if (scope_guard.primary_guard) {
    attribution_override = location;
  }

  return scope_guard;
}

auto Diagnostics::Log::log(
    Level level,
    View::Bytes message,
    const Source& location) -> void {
  if (level < thread_log_level || !message_sink) {
    return;
  }

  const Source& target_source =
      attribution_override.is_set() ? attribution_override : location;
  message_sink(level, message, target_source);
}

auto Diagnostics::Log::format_entry(
    Log::Level level,
    View::Bytes message,
    const Source& location,
    Access::Bytes output) -> Count {
  Writer::Textual writer(output);

  writer << level_char(level) << ' ';
  writer << Time::now().calculate_clock() << ' ';
  writer << "["_view << Thread::Worker::get_thread_name() << "] "_view;

  writer << location.get_file();
  writer << ':' << location.get_line();
  writer << ':' << location.get_column();
  writer << ": "_view << message << '\n';
  return writer.get_location();
}

auto Diagnostics::Log::debug(View::Bytes message, const Source& location)
    -> void {
  log(Level::Debug, message, location);
}

auto Diagnostics::Log::info(View::Bytes message, const Source& location)
    -> void {
  log(Level::Info, message, location);
}

auto Diagnostics::Log::warning(View::Bytes message, const Source& location)
    -> void {
  log(Level::Warning, message, location);
}

auto Diagnostics::Log::error(View::Bytes message, const Source& location)
    -> void {
  log(Level::Error, message, location);
}

auto Diagnostics::Log::fatal(View::Bytes message, const Source& location)
    -> void {
  log(Level::Fatal, message, location);
  flush();

#ifdef PERI_LINUX
  Static::Vector<void*, 64> frames;
  int frame_count = backtrace(frames.get_data(), frames.get_size());
  char** symbols = backtrace_symbols(frames.get_data(), frame_count);
  if (symbols) {
    // Fatal diagnostics obey the same selected sink as every other toolchain
    // message. Writing the trace directly to descriptor 2 would bypass a
    // file only host policy and leak internal failures into user diagnostics.
    for (int index = 0; index < frame_count; index++) {
      log(Level::Fatal, NullTerminated::to_view(symbols[index]), Source());
    }
    free(symbols);
  } else {
    log(Level::Fatal,
        "The platform could not symbolize the fatal backtrace."_view, Source());
  }
  flush();
#endif

  abort();
}

auto Diagnostics::Log::flush() -> void {
  thread_writer.flush();
}
