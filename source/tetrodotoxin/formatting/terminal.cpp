// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/formatting/terminal.hpp"

#include "tetrodotoxin/source/lexical/formatter.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

static auto trim_payload(View::Bytes line, Count marker) -> View::Bytes {
  Count opening = marker + 2;
  while (opening < line.get_size() &&
         (line[opening] == ' ' || line[opening] == '\t')) {
    opening++;
  }
  Count closing = line.get_size();
  while (closing > opening &&
         (line[closing - 1] == ' ' || line[closing - 1] == '\t')) {
    closing--;
  }
  return line.slice(opening, closing - opening);
}

static auto is_structured(View::Bytes payload) -> Bool {
  if (payload.is_empty()) {
    return True;
  }
  U8 first = payload[0];
  if (first == '`' || first == '#' || first == '>' || first == '*' ||
      first == '-') {
    return True;
  }
  return payload.get_size() > 1 && first >= '0' && first <= '9' &&
         payload[1] == '.';
}

static auto find_marker(View::Bytes line) -> Count {
  Count marker = 0;
  while (marker < line.get_size() && line[marker] == ' ') {
    marker++;
  }
  return marker + 1 < line.get_size() && line[marker] == '/' &&
                 line[marker + 1] == '/' &&
                 (marker + 2 == line.get_size() || line[marker + 2] != '/')
             ? marker
             : Count(-1);
}

static auto append_wrapped(
    Dynamic::Bytes& output,
    Count indent,
    View::Bytes paragraph) -> void {
  constexpr Count width = 88;
  Count prefix_size = indent + 3;
  Count line_size = prefix_size;
  output.append(' ', indent);
  output.concat("// "_view);

  Count cursor = 0;
  Bool first = True;
  while (cursor < paragraph.get_size()) {
    while (cursor < paragraph.get_size() && paragraph[cursor] == ' ') {
      cursor++;
    }
    Count opening = cursor;
    while (cursor < paragraph.get_size() && paragraph[cursor] != ' ') {
      cursor++;
    }
    View::Bytes word = paragraph.slice(opening, cursor - opening);
    if (word.is_empty()) {
      continue;
    }

    Count required = word.get_size() + (first ? 0 : 1);
    if (!first && line_size + required > width) {
      output.append('\n');
      output.append(' ', indent);
      output.concat("// "_view);
      line_size = prefix_size;
      first = True;
    }
    if (!first) {
      output.append(' ');
      line_size++;
    }
    output.concat(word);
    line_size += word.get_size();
    first = False;
  }
}

static auto reflow_comments(View::Bytes source) -> Dynamic::Bytes {
  Dynamic::Bytes output(source.get_size());
  Count cursor = 0;
  while (cursor < source.get_size()) {
    Count line_end = cursor;
    while (line_end < source.get_size() && source[line_end] != '\n') {
      line_end++;
    }
    View::Bytes line = source.slice(cursor, line_end - cursor);
    Count marker = find_marker(line);
    View::Bytes payload =
        marker == Count(-1) ? View::Bytes() : trim_payload(line, marker);
    if (marker == Count(-1) || is_structured(payload)) {
      output.concat(line);
      if (line_end < source.get_size()) {
        output.append('\n');
      }
      cursor = line_end + (line_end < source.get_size() ? 1 : 0);
      continue;
    }

    Dynamic::Bytes paragraph(payload);
    Count next = line_end + (line_end < source.get_size() ? 1 : 0);
    while (next < source.get_size()) {
      Count next_end = next;
      while (next_end < source.get_size() && source[next_end] != '\n') {
        next_end++;
      }
      View::Bytes next_line = source.slice(next, next_end - next);
      Count next_marker = find_marker(next_line);
      View::Bytes next_payload = next_marker == Count(-1)
                                     ? View::Bytes()
                                     : trim_payload(next_line, next_marker);
      if (next_marker != marker || is_structured(next_payload)) {
        break;
      }
      paragraph.append(' ');
      paragraph.concat(next_payload);
      next = next_end + (next_end < source.get_size() ? 1 : 0);
    }

    append_wrapped(output, marker, paragraph.get_view());
    if (next <= source.get_size() &&
        (next != source.get_size() || source[source.get_size() - 1] == '\n')) {
      output.append('\n');
    }
    cursor = next;
  }
  return output;
}

auto Formatting::Terminal::format(
    const Language::Monograph& monograph,
    const Tetrodotoxin::Source::Lexical::Tokenizer& tokenizer) -> Dynamic::Bytes {
  (void)monograph;
  Dynamic::Bytes canonical = Tetrodotoxin::Source::Lexical::Formatter(tokenizer).format();
  return reflow_comments(canonical.get_view());
}
