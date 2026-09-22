// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/args.hpp"

#ifdef PERI_LINUX
#include <unistd.h>
#endif

#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;

static auto basename(View::Bytes path) -> View::Bytes {
  for (Count i = path.get_size(); i > 0; i--) {
    if (path[i - 1] == '/' || path[i - 1] == '\\') {
      return path.slice(i);
    }
  }

  return path;
}

static auto process_name() -> View::Bytes {
#ifdef PERI_LINUX
  static U8 path_buffer[512];
  S64 size = readlink(
      "/proc/self/exe", Data::cast<char>(path_buffer), sizeof(path_buffer));
  if (size > 0) {
    return basename(View::Bytes(path_buffer, Count(size)));
  }
#endif
  return "process"_view;
}

// Parses the argument and returns a name / value pair.
static auto parse_argument(View::Bytes argument)
    -> Pair<View::Bytes, View::Bytes> {
  // Extract positions
  if (argument.is_empty() || argument[0] != '-') {
    return {View::Bytes(), argument};
  }

  while (!argument.is_empty() && argument[0] == '-') {
    argument = argument.slice(1);
  }

  Count equals = Algorithm::search(argument, "="_view);
  if (equals == Count(-1)) {
    return {argument, "true"_view};
  } else {
    return {argument.slice(0, equals), argument.slice(equals + 1)};
  }
}

static auto format_help(
    Allocator::Arena& arena,
    View::Bytes summary,
    const Managed::Map<View::Bytes, View::Bytes>& variables,
    View::Bytes command) -> View::Bytes {
  Count label_width = "-help"_view.get_size();
  for (Count i = 0; i < variables.get_size(); i++) {
    const auto* variable = variables.get_entry(i);
    label_width = Math::max(label_width, variable->key.get_size() + 1);
  }

  label_width += 2;

  Managed::Bytes output(arena);
  output.concat("usage: "_view);
  output.concat(command);
  output.concat(" [arguments]\n\n"_view);
  if (!summary.is_empty()) {
    output.concat(summary);
    output.concat("\n\n"_view);
  }

  output.concat("arguments:\n"_view);
  for (Count i = 0; i < variables.get_size(); i++) {
    const auto* variable = variables.get_entry(i);

    output.concat("  -"_view);
    output.concat(variable->key);
    output.append(U8(' '), label_width - variable->key.get_size() - 1);
    output.concat(variable->value);
    output.append('\n');
  }

  output.concat("  -help"_view);
  output.append(U8(' '), label_width - "-help"_view.get_size());
  output.concat("Show this help.\n"_view);
  return output;
}

auto Args::parse(
    Allocator::Arena& arena,
    const Managed::Map<View::Bytes, View::Bytes>& variables,
    View::Vector<View::Bytes> arguments) -> Values {
  Values values(arena);
  const auto* argument_data = arguments.get_data();
  for (Count i = 1; i < arguments.get_size(); i++) {
    auto argument = parse_argument(argument_data[i]);
    if (argument.key != "help"_view && !variables.contains(argument.key)) {
      Diagnostics::Log::Message<256> error_message(
          Diagnostics::Log::Level::Error);
      error_message << "unrecognized arg"_view << ' ' << argument_data[i]
                    << '\n';
      return Values(arena);
    }

    if (!values.contains(argument.key)) {
      values[argument.key] =
          &arena.construct<Managed::Vector<View::Bytes>>(arena);
    }

    values[argument.key]->insert(argument.value);
  }

  return values;
}

auto Args::log_help(
    Allocator::Arena& arena,
    View::Bytes summary,
    const Managed::Map<View::Bytes, View::Bytes>& variables,
    View::Vector<View::Bytes> arguments) -> void {
  View::Bytes command;
  if (!arguments.is_empty() && !arguments.get_data()[0].is_empty()) {
    command = basename(arguments.get_data()[0]);
  } else {
    command = process_name();
  }

  Diagnostics::Log::info(format_help(arena, summary, variables, command));
}
