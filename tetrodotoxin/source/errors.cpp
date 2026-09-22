// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/errors.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Source;

auto Errors::report(
    View::Bytes name,
    View::Bytes text,
    Option<Anchor> anchor,
    View::Bytes message,
    View::Bytes hint) -> void {
  // Reuse only the exact snapshot, rather than treating a filename as an
  // observation identity. A second revision may share its path with the first.
  View::Bytes retained;
  Bool found = False;
  for (const auto& error : errors.get_view()) {
    if (error.name == name && error.text == text) {
      name = error.name;
      retained = error.text;
      found = True;
      break;
    }
  }

  if (!found) {
    name = arena.proxy(name);
    retained = arena.proxy(text);
  }

  errors.insert(Error(
      Diagnostic(anchor, arena.proxy(message), arena.proxy(hint)), name,
      retained));
}

auto Errors::get_error(Count index) const -> Option<Diagnostic> {
  if (index >= errors.get_size()) {
    return {};
  }
  return errors.at(index).diagnostic;
}

static auto line_at(View::Bytes text, Count offset) -> Count {
  Count line = 1;
  for (Count i = 0; i < offset; ++i) {
    line += text[i] == '\n';
  }
  return line;
}

static auto line_start(View::Bytes text, Count offset) -> Count {
  while (offset && text[offset - 1] != '\n') {
    --offset;
  }
  return offset;
}

static auto inside(View::Bytes text, Range range) -> Bool {
  return range.get_offset() <= text.get_size() &&
         range.get_size() <= text.get_size() - range.get_offset();
}

// Source ranges retain bytes, not cached line numbers. Diagnostic rendering is
// the cold path where those presentation facts are needed. Computing them from
// the copied snapshot also makes multiline tokens and revised files agree with
// the exact bytes the error context retained.
auto Errors::render_message(
    Allocator::Arena& arena,
    Count index,
    View::Bytes display_name) const -> View::Bytes {
  if (index >= errors.get_size()) {
    return {};
  }
  const auto& error = errors.at(index);
  const auto name = display_name.is_empty() ? error.name : display_name;
  const auto text = error.text;
  auto anchor = error.diagnostic.get_anchor();
  if (anchor && !inside(text, anchor->get_extent())) {
    anchor = {};
  }
  Option<Range> focus;
  if (anchor) {
    const auto candidate = anchor->get_focus();
    const auto extent = anchor->get_extent();
    if (candidate && inside(text, *candidate) &&
        candidate->get_offset() >= extent.get_offset() &&
        candidate->get_offset() + candidate->get_size() <=
            extent.get_offset() + extent.get_size()) {
      focus = candidate;
    }
  }

  Managed::Bytes message(arena);
  Stream::Textual<Managed::Bytes> render(message);
  render << "\x1b[38;2;227;62;60m\x1b[1m[ERROR] "_view << name << ":"_view;
  if (anchor) {
    const auto coordinate = focus ? *focus : anchor->get_extent();
    render << line_at(text, coordinate.get_offset()) << ":"_view
           << coordinate.get_offset() -
                  line_start(text, coordinate.get_offset()) + 1
           << ":"_view;
  }
  render << "\n\x1b[0m"_view << error.diagnostic.get_message() << "\n"_view;

  if (anchor) {
    const auto extent = anchor->get_extent();
    Count start = line_start(text, extent.get_offset());
    Count end = extent.get_offset() + extent.get_size();
    while (end < text.get_size() && text[end] != '\n') {
      ++end;
    }
    Count line = line_at(text, start);
    do {
      Count next = start;
      while (next < end && text[next] != '\n') {
        ++next;
      }
      Count digits = 1;
      for (Count value = line; value >= 10; value /= 10) {
        ++digits;
      }
      message.append(' ', digits < 5 ? 5 - digits : 0);
      render << line << " | "_view << text.slice(start, next - start)
             << "\n"_view;
      if (focus && focus->get_offset() >= start &&
          focus->get_offset() <= next) {
        render << "      | "_view;
        message.append(' ', focus->get_offset() - start);
        render << "^"_view;
        const Count available = next - focus->get_offset();
        const Count width =
            focus->get_size() < available ? focus->get_size() : available;
        if (width > 1) {
          message.append('-', width - 1);
        }
        render << "\n"_view;
      }
      start = next + 1;
      ++line;
    } while (start <= end);
  }

  if (!error.diagnostic.get_hint().is_empty()) {
    render << "Note: "_view << error.diagnostic.get_hint() << "\n"_view;
  }
  render << "\n\x1b[0m"_view;
  return message;
}
