// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "perimortem/core/static/vector.hpp"

#include "toolchain/validation/process/child.hpp"

using namespace Perimortem::Core;
using namespace Toolchain::Validation;

static auto read(const std::filesystem::path& path) -> std::string {
  std::ifstream file(path);
  return std::string(
      std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

// This process test crosses Puffer, native Build, Source and two independently
// loaded terminals. Their files prove that Build gives every discovered Export
// the complete workspace. The refusal cases exercise publication and routing
// boundaries using the same module inventory rather than separate toy builds.
auto main(int argc, char** argv) -> int {
  const char* temporary = std::getenv("TEST_TMPDIR");
  if (argc != 9 || !temporary) {
    return 2;
  }

  const std::filesystem::path directory(temporary);
  const auto first = directory / "first.txt";
  const auto second = directory / "second.txt";
  Static::Vector<Bytes, 20> arguments = {{
    bytes(argv[2]),

    bytes("--module"), bytes("source"),  bytes(argv[3]),

    bytes("--module"), bytes("first"),   bytes(argv[5]),

    bytes("--module"), bytes("second"),  bytes(argv[6]),

    bytes("--source"), bytes("main"),    bytes(argv[4]), bytes("source"),

    bytes("--source"), bytes("support"), bytes(argv[4]), bytes("source"),

    bytes("--output"), bytes(temporary),
  }};
  const auto run = [&] {
    return Process::run(
        Process::Request(
            bytes(argv[1]), arguments.get_data(), arguments.get_size(), Bytes(),
            10'000'000'000));
  };

  // Both exporters must observe both named roots, and Puffer reports success
  // only after their output is complete. Comparing the files also catches a
  // dispatcher that invokes one terminal twice while skipping the other.
  {
    Process::Expectation expected;
    expected.standard_output = bytes("Build complete\n");

    const auto success = run();
    const auto difference = Process::compare(success, expected);
    const auto product = read(first);
    if (difference != Process::Difference::None || product != read(second) ||
        product.find("main\t7\n") == std::string::npos ||
        product.find("support\t7\n") == std::string::npos) {
      return 3;
    }
  }

  std::error_code error;
  std::filesystem::remove(first, error);
  std::filesystem::remove(second, error);

  // Source supplies malformed lexical evidence, but these terminals decline
  // it before writing. Keeping the first member valid proves that admission
  // considers the whole workspace before either fixture publishes a file.
  arguments[16] = bytes(argv[7]);
  {
    const auto rejected = run();
    if (!rejected.launched || rejected.timed_out || rejected.exit_status != 1 ||
        std::filesystem::exists(first) || std::filesystem::exists(second)) {
      return 4;
    }
  }

  // An unreadable second member leaves the import group incomplete. Build
  // must stop before invoking terminals even though the first root is ready.
  arguments[16] = bytes("missing-input.ttx");
  {
    const auto missing = run();
    if (!missing.launched || missing.timed_out || missing.exit_status != 1 ||
        std::filesystem::exists(first) || std::filesystem::exists(second)) {
      return 5;
    }
  }

  // Names identify workspace members. A duplicate must produce a diagnostic
  // instead of silently replacing the first root or exporting an ambiguous one.
  arguments[16] = bytes(argv[4]);
  arguments[15] = bytes("main");
  {
    const auto duplicate = run();
    const std::string_view diagnostic(
        static_cast<const char*>(duplicate.standard_error.data),
        duplicate.standard_error.size);
    if (!duplicate.launched || duplicate.timed_out ||
        duplicate.exit_status != 1 ||
        diagnostic.find("Duplicate source name") == std::string_view::npos ||
        std::filesystem::exists(first) || std::filesystem::exists(second)) {
      return 6;
    }
  }

  // An authored importer name must select an installed module. The available
  // Source module cannot be used as an implicit fallback for a missing name.
  arguments[15] = bytes("support");
  arguments[17] = bytes("missing");
  {
    const auto unknown = run();
    if (!unknown.launched || unknown.timed_out || unknown.exit_status != 1 ||
        std::filesystem::exists(first) || std::filesystem::exists(second)) {
      return 7;
    }
  }

  // Successful imports alone do not finish a build. With no Export capability
  // installed, Build must report the missing terminal rather than succeed with
  // an empty set of outputs.
  {
    const Static::Vector<Bytes, 10> no_terminal_arguments = {{
      bytes(argv[2]),
      bytes("--module"),
      bytes("source"),
      bytes(argv[3]),
      bytes("--source"),
      bytes("main"),
      bytes(argv[4]),
      bytes("source"),
      bytes("--output"),
      bytes(temporary),
    }};
    const auto no_terminal = Process::run(
        Process::Request(
            bytes(argv[1]), no_terminal_arguments.get_data(),
            no_terminal_arguments.get_size(), Bytes(), 10'000'000'000));
    const std::string_view diagnostic(
        static_cast<const char*>(no_terminal.standard_error.data),
        no_terminal.standard_error.size);
    if (!no_terminal.launched || no_terminal.timed_out ||
        no_terminal.exit_status != 1 ||
        diagnostic.find("No terminal capability") == std::string_view::npos ||
        std::filesystem::exists(first) || std::filesystem::exists(second)) {
      return 8;
    }
  }

  // A valid Import may return a root without Borrow. Build cannot keep that
  // root for its later terminal phase and must refuse it before exporting.
  arguments[17] = bytes("source");
  arguments[3] = bytes(argv[8]);
  {
    const auto unretained = run();
    if (!unretained.launched || unretained.timed_out ||
        unretained.exit_status != 1 || std::filesystem::exists(first) ||
        std::filesystem::exists(second)) {
      return 9;
    }
  }

  // The same provider also exposes Puffer's own acceptance boundary. Loading
  // an Import capability is insufficient unless its result supplies the
  // retained Build Workspace that Puffer promises to keep alive.
  arguments[0] = bytes(argv[8]);
  {
    const auto no_workspace = run();
    return no_workspace.launched && !no_workspace.timed_out &&
                   no_workspace.exit_status == 1
               ? 0
               : 10;
  }
}
