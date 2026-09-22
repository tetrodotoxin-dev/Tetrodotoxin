// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// This should only be included in cpp files. Including it in a header would
// pull source location machinery into every translation unit that includes it.

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/diagnostics/source.hpp"
#include "perimortem/core/writer/textual.hpp"

namespace Perimortem::Core::Diagnostics {

class Log {
 public:
  enum class Level : U8 {
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
  };

  // Debug logs often come from deep in the framework while the actual context
  // that the message is useful in is some higher caller on the stack.
  // Attribution points allow for overriding source attribution for all callee
  // logs.
  class Attribution {
    friend Log;

   public:
    Attribution(Attribution&&);
    ~Attribution();

   private:
    Attribution() = default;

    Bool primary_guard = False;
  };

  // Creates a scoped log message that emits when it leaves the block.
  // This keeps log sites focused on the message instead of repeating temporary
  // byte buffer and textual writer ceremony.
  template <Count capacity>
  class Message {
   public:
    Message(
        Level message_level,
        const Source& message_location = Source::current())
        : writer(message_storage.get_access()),
          level(message_level),
          location(message_location) {}
    ~Message() {
      View::Bytes payload = get_message();
      if (payload.is_empty()) {
        return;
      }

      if (level == Level::Fatal) {
        Log::fatal(payload, location);
        return;
      }

      Log::log(level, payload, location);
    }

    Message(const Message&) = delete;
    Message(Message&&) = delete;
    auto operator=(const Message&) -> Message& = delete;
    auto operator=(Message&&) -> Message& = delete;

    template <typename value_type>
    auto operator<<(const value_type& value) -> Message& {
      writer << value;
      return *this;
    }

    constexpr auto get_message() const -> View::Bytes {
      return message_storage.slice(0, writer.get_location());
    }

   private:
    Static::Bytes<capacity> message_storage;
    Writer::Textual writer;
    Level level;
    Source location;
  };

  // Receives the raw log message and resolved source attribution.
  // The sink function is local to each thread.
  using Sink =
      void (*)(Level level, Core::View::Bytes message, const Source& location);

  // The default sink function the logger uses for each thread.
  // Creates a canonical file for the thread the first time sink is called
  // for the thread.
  static auto file_sink(
      Level level,
      Core::View::Bytes message,
      const Source& location) -> void;

  // Causes the thread to only log messages to the console (stdout / stderr).
  // Writes to the console are thread safe to prevent multiple threads from
  // creating garbage output.
  // Debug, Info and Warning are written to stdout.
  // Error and Fatal log to stderr.
  static auto console_sink(
      Level level,
      Core::View::Bytes message,
      const Source& location) -> void;

  // Writes only the supplied message to the selected console stream. This is
  // for diagnostics that already own their complete source presentation.
  static auto plain_sink(
      Level level,
      Core::View::Bytes message,
      const Source& location) -> void;

  // Uses the console sink but writes colored text.
  // Grey for debug, white for info, yellow for warning, red for error and
  // fatal.
  static auto color_sink(
      Level level,
      Core::View::Bytes message,
      const Source& location) -> void;

  // Writes all log levels to stderr. Flushes after every message.
  // Use when stdout is a protocol pipe (e.g. LSP server).
  static auto stderr_sink(
      Level level,
      Core::View::Bytes message,
      const Source& location) -> void;

  // Logs to both the console_sink and the file_sink for real time and
  // persistent logs.
  static auto debug_sink(
      Level level,
      Core::View::Bytes message,
      const Source& location) -> void;

  // Sets the function to forward log messages to on this thread.
  // By default the sink is set to a file on disk named after the thread.
  // If the sink is set to null then messages for the thread are dropped.
  static auto set_sink(Sink sink) -> void;
  static auto get_sink() -> Sink;

  // Disables the standard log header on the current thread. Diagnostics that
  // already own their display format should use this instead of a parallel raw
  // sink.
  static auto set_disable_header(Bool disable_header) -> void;
  static auto get_disable_header() -> Bool;

  // Sets the minimum level that will be forwarded to the sink on this thread.
  // Messages below this level are dropped before formatting.
  static auto set_level(Level level) -> void;
  static auto get_level() -> Diagnostics::Log::Level;

  // Sets logs in the current scope
  static auto set_attribution(const Source& location = Source::current())
      -> Attribution;

  // Default sink to use.
  static constexpr Sink default_sink = file_sink;

  // Logging calls are thread safe
  // Each thread writes to its own buffer and file.
  // The Source default argument is consteval, so it captures the caller's
  // location at compile time with zero runtime cost.
  // Source is passed by reference to preserve a simple cross language ABI.
  static auto log(
      Level level,
      Core::View::Bytes message,
      const Source& location) -> void;

  static auto debug(
      Core::View::Bytes message,
      const Source& location = Source::current()) -> void;
  static auto info(
      Core::View::Bytes message,
      const Source& location = Source::current()) -> void;
  static auto warning(
      Core::View::Bytes message,
      const Source& location = Source::current()) -> void;
  static auto error(
      Core::View::Bytes message,
      const Source& location = Source::current()) -> void;

  // Fatal logs the message and platform backtrace through the selected sink,
  // flushes it, then aborts. Marking it [[noreturn]] preserves fatal call
  // paths.
  [[noreturn]] static auto fatal(
      Core::View::Bytes message,
      const Source& location = Source::current()) -> void;

  // Flushes the calling thread's message buffer.
  // Stdout writes are guaranteed to be thread safe to avoid output mangling.
  // Flush is called automatically whenever the message buffer is close to being
  // filled or when the thread exits.
  static auto flush() -> void;

  // Used by custom loggers to format output with the optional standardized
  // Perimortem header information.
  static auto format_entry(
      Level level,
      Core::View::Bytes message,
      const Source& location,
      Core::Access::Bytes buffer) -> Count;
};

}  // namespace Perimortem::Core::Diagnostics
