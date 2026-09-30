// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialects/source/errors.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Dialects::Source;

// The original TTX diagnostic palette gives each part of the message a stable
// visual role: red for the error identity, orange for source context, and
// yellow for the exact range and suggested resolution.
static constexpr View::Bytes clear_color = "\x1b[0m"_view;
static constexpr View::Bytes bold = "\x1b[1m"_view;
static constexpr View::Bytes italic = "\x1b[3m"_view;
static constexpr View::Bytes error_primary = "\x1b[38;2;227;62;60m"_view;
static constexpr View::Bytes error_secondary = "\x1b[38;2;222;122;101m"_view;
static constexpr View::Bytes error_tertiary = "\x1b[38;2;245;147;85m"_view;
static constexpr View::Bytes error_highlight = "\x1b[38;2;255;201;107m"_view;
static constexpr View::Bytes source_color = "\x1b[38;2;255;102;102m"_view;

Errors::Report::Report(
    Errors& errors,
    View::Bytes source_name,
    View::Bytes source_text,
    Anchor anchor)
    : errors(errors),
      source_name(source_name),
      source_text(source_text),
      anchor(anchor),
      message_storage(errors.arena),
      hint_storage(errors.arena),
      message(message_storage),
      hint(hint_storage) {}

Errors::Report::~Report() {
  if (message_storage.get_size() == 0) {
    return;
  }

  errors.publish_report(
      source_name, source_text, message_storage, hint_storage, anchor);
}

auto Errors::retain_source(View::Bytes source_name, View::Bytes source_text)
    -> View::Bytes {
  auto retained = source_map.find(source_name);
  if (retained) {
    return (*retained).key;
  }

  View::Bytes retained_name = arena.proxy(source_name);
  View::Bytes retained_text = arena.proxy(source_text);
  source_map.insert(retained_name, retained_text);
  return retained_name;
}

auto Errors::publish_report(
    View::Bytes source_name,
    View::Bytes source_text,
    View::Bytes message,
    View::Bytes hint,
    Anchor anchor) -> void {
  // Report message storage already belongs to this Arena. Retain or recover the
  // canonical source name without copying a source body already retained here.
  errors.insert({
    .message = message,
    .hint = hint,
    .source_name = retain_source(source_name, source_text),
    .anchor = anchor,
  });
}

// Spans store byte offsets rather than owning source lines. Expand the source
// range to the surrounding line boundaries so every affected line can be
// rendered with its own gutter while still borrowing the stored source text.
static auto source_range(View::Bytes source, Span span) -> View::Bytes {
  // A general report carries no authored coordinates. Keep its excerpt empty
  // instead of reading the unspecified coordinates of an invalid Span.
  BAIL_IF(!span);

  Count span_start = span.get_offset();
  Count span_end = span_start + span.get_size();
  BAIL_IF(span_start > source.get_size() || span_end > source.get_size());

  // The opening Token may sit in the middle of a line. Recover the preceding
  // bytes so the first gutter still presents the complete authored line.
  Count start = span.get_offset();
  while (start != 0 && source[start - 1] != '\n') {
    start--;
  }

  // The closing Token may also sit before the line ending. Retaining the
  // trailing bytes gives the final affected line the same complete context.
  Count end = span.get_offset() + span.get_size();
  while (end < source.get_size() && source[end] != '\n') {
    end++;
  }

  return source.slice(start, end - start);
}

// Anchor deliberately permits an empty or external Token so semantic owners
// can forward the best source range they have. Rendering adds a caret only
// when the complete Token is inside both that Span and the retained source.
static auto has_visible_caret(View::Bytes source, Anchor anchor) -> Bool {
  Span span = anchor.get_span();
  Token token = anchor.get_token();
  BAIL_IF(!span || !token || token.get_size() == 0);

  Count span_start = span.get_offset();
  Count span_end = span_start + span.get_size();
  Count token_start = token.get_offset();
  Count token_end = token_start + token.get_size();
  BAIL_IF(
      span_end > source.get_size() || token_start < span_start ||
      token_end > span_end);

  return token.get_line() >= span.get_start().get_line() &&
         token.get_line() <= span.get_end().get_line() &&
         token.get_column() != 0;
}

// Source gutters reserve five columns for the line number aligned on the
// right. Large line numbers grow past the gutter instead of being truncated.
static auto decimal_digits(Count value) -> Count {
  Count digits = 1;
  while (value >= 10) {
    value /= 10;
    digits++;
  }
  return digits;
}

static auto write_source_gutter(
    Managed::Bytes& message,
    Perimortem::Serialization::Stream::Textual<Managed::Bytes>& render,
    Count line) -> void {
  render << error_highlight;
  Count digits = decimal_digits(line);
  if (digits < 5) {
    message.append(' ', 5 - digits);
  }
  render << line << error_tertiary << " | "_view;
}

// The caret belongs to the source excerpt but not to a source line, so it gets
// the same separator with an intentionally empty line number field.
static auto write_caret_gutter(
    Perimortem::Serialization::Stream::Textual<Managed::Bytes>& render)
    -> void {
  render << error_tertiary << "      | "_view << error_highlight;
}

// Renders the following diagnostic shape into the caller's arena (colors are
// omitted here for readability):
//
//   [ERROR] source.ttx:12:5:
//   Explanation of the error
//      12 | first affected source line
//         |     ^ plus its underline
//      13 | final affected source line
//   Note: optional recovery hint
//
// General errors stop after the explanation because they have no Span.
// An index outside the retained reports produces an empty view.
auto Errors::render_message(
    Perimortem::Memory::Allocator::Arena& arena,
    Count index,
    View::Bytes display_source_name) const -> Perimortem::Core::View::Bytes {
  Managed::Bytes message(arena);
  Perimortem::Serialization::Stream::Textual<Managed::Bytes> render(message);

  BAIL_IF(index >= errors.get_size());

  const Error& error = errors.at(index);
  auto source = source_map.find(error.source_name);
  BAIL_IF(!source);

  View::Bytes source_name =
      display_source_name.is_empty() ? (*source).key : display_source_name;
  View::Bytes source_text = (*source).value;
  const Span span = error.anchor.get_span();
  const Token token = error.anchor.get_token();
  const Bool visible_caret = has_visible_caret(source_text, error.anchor);
  const Count token_size = visible_caret ? token.get_size() : 0;
  const Count span_line_count = span ? span.get_line_count() : 0;

  // Span controls the complete excerpt while Token independently chooses the
  // diagnostic coordinate and marker width within that source context.
  View::Bytes range = source_range(source_text, span);

  // One reservation covers the borrowed text plus gutters and color escapes.
  // General reports contribute zero Span bytes without reading coordinates.
  message.reset(
      source_name.get_size() + error.message.get_size() +
      error.hint.get_size() + range.get_size() + token_size +
      (span_line_count * 48) + 256);

  // A general report still names its source, but its invalid Span deliberately
  // omits line and column coordinates.
  render << error_primary << bold << "[ERROR] "_view << error_secondary
         << italic << source_color << source_name << ":"_view;
  if (visible_caret) {
    render << token.get_line() << ":"_view << token.get_column() << ":"_view;
  } else if (span) {
    render << span.get_line() << ":"_view << span.get_column() << ":"_view;
  }
  render << "\n"_view << clear_color;

  render << error_secondary << bold << error.message << "\n"_view;

  if (span) {
    // Span retains coordinates rather than line slices. Splitting the expanded
    // range here keeps empty lines visible and every gutter aligned.
    Count line_number = span.get_line();
    Count line_start = 0;
    for (Count i = 0; i <= range.get_size(); i++) {
      if (i != range.get_size() && range[i] != '\n') {
        continue;
      }

      // A source range excludes its final newline, so reaching the View end is
      // the only reliable way to publish its last character or empty line.
      write_source_gutter(message, render, line_number);
      render << range.slice(line_start, i - line_start) << "\n"_view;

      // Place the marker beside its own source line. A later line in a broad
      // expression Span must not separate the operator from its diagnostic.
      if (visible_caret && line_number == token.get_line()) {
        write_caret_gutter(render);
        if (token.get_column() > 1) {
          message.append(' ', token.get_column() - 1);
        }

        render << "^"_view;
        if (token_size > 1) {
          message.append('-', token_size - 1);
        }
        render << "\n"_view;
      }

      line_number++;
      line_start = i + 1;
    }
  }

  if (!error.hint.is_empty()) {
    render << error_tertiary << "Note: "_view << error_highlight << error.hint
           << "\n"_view;
  }

  // Never leak the diagnostic styling into subsequent terminal output.
  render << "\n"_view << clear_color;
  return message;
}

auto Errors::report(
    tetrodotoxin_source_snapshot input,
    Anchor anchor,
    View::Bytes message,
    View::Bytes hint) -> void {
  Report report(
      *this, {input.path.data, input.path.size},
      {input.text.data, input.text.size}, anchor);
  report << message;
  report.get_hint() << hint;
}

auto Errors::supports(Perimortem::System::Uuid id) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  return id == Capabilities::Diagnostics::contract_id
             ? Ttx::Semantic::Negotiation::Binding::Status::Satisfied
             : Ttx::Semantic::Negotiation::Binding::Status::Unknown;
}

auto Errors::bind_interface(
    Perimortem::System::Uuid id,
    Ttx::Data::Form::Storage target) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  if (id == Capabilities::Diagnostics::contract_id) {
    return Ttx::Semantic::Negotiation::Binding::provide<
        Capabilities::Diagnostics>(
        const_cast<Errors*>(this)->get_diagnostics().get_abi(), target);
  }
  return Ttx::Semantic::Negotiation::Binding::Status::Unknown;
}
