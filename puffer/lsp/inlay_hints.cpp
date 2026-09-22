// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/inlay_hints.hpp"

#include "perimortem/core/math.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/json/blueprint.hpp"

#include "tetrodotoxin/source/callable.hpp"

using namespace Perimortem;
using namespace Puffer;

static auto skip_space(Core::View::Bytes source, Count offset, Count end)
    -> Count {
  while (offset < end && (source[offset] == ' ' || source[offset] == '\t' ||
                          source[offset] == '\r' || source[offset] == '\n')) {
    offset++;
  }
  return offset;
}

static auto collect_arguments(
    Core::View::Bytes source,
    Tetrodotoxin::Source::Lexical::Anchor anchor,
    Memory::Managed::Vector<Count>& arguments) -> Bool {
  Tetrodotoxin::Source::Lexical::Token focus = anchor.get_token();
  Tetrodotoxin::Source::Lexical::Span span = anchor.get_span();
  BAIL_IF(!focus || !span);
  Count end = Core::Math::min(
      source.get_size(), Count(span.get_offset()) + span.get_size());
  Count offset = Count(focus.get_offset()) + focus.get_size();
  offset = skip_space(source, offset, end);
  BAIL_IF(offset >= end || source[offset] != '(');

  offset = skip_space(source, offset + 1, end);
  if (offset >= end || source[offset] == ')') {
    return True;
  }
  arguments.insert(offset);

  Count parentheses = 1;
  Count brackets = 0;
  Count braces = 0;
  Bool quoted = False;
  Bool escaped = False;
  Bool comment = False;
  for (; offset < end; offset++) {
    U8 byte = source[offset];
    if (comment) {
      if (byte == '\n') {
        comment = False;
      }
      continue;
    }
    if (quoted) {
      if (escaped) {
        escaped = False;
      } else if (byte == '\\') {
        escaped = True;
      } else if (byte == '"') {
        quoted = False;
      }
      continue;
    }
    if (byte == '"') {
      quoted = True;
      continue;
    }
    if (byte == '/' && offset + 1 < end && source[offset + 1] == '/') {
      comment = True;
      offset++;
      continue;
    }
    if (byte == '(') {
      parentheses++;
    } else if (byte == ')') {
      BAIL_IF(parentheses == 0);
      parentheses--;
      if (parentheses == 0) {
        return True;
      }
    } else if (byte == '[') {
      brackets++;
    } else if (byte == ']') {
      BAIL_IF(brackets == 0);
      brackets--;
    } else if (byte == '{') {
      braces++;
    } else if (byte == '}') {
      BAIL_IF(braces == 0);
      braces--;
    } else if (
        byte == ',' && parentheses == 1 && brackets == 0 && braces == 0) {
      Count next = skip_space(source, offset + 1, end);
      BAIL_IF(next >= end || source[next] == ')');
      arguments.insert(next);
    }
  }
  return False;
}

auto Puffer::Lsp::inlay_hints_for(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes source,
    const PositionEncoding& encoding,
    const Tetrodotoxin::Source::Lexical::Associations& associations,
    const PositionEncoding::Position& start,
    const PositionEncoding::Position& end) -> Serialization::Json::Node {
  Memory::Managed::Vector<Serialization::Json::Node> hints(arena);
  // An exact Association retains the selected Callable, while its Anchor keeps
  // the authored call span. The Callable's ordered Layout is the sole semantic
  // parameter authority; the lexical span contributes only argument positions.
  for (const Tetrodotoxin::Source::Lexical::Associations::Entry& association :
       associations.get_entries()) {
    auto callable = association.get_semantic().select<Tetrodotoxin::Source::Callable>();
    if (!callable) {
      continue;
    }

    Memory::Managed::Vector<Count> arguments(arena);
    if (!collect_arguments(source, association.get_anchor(), arguments) ||
        arguments.is_empty()) {
      continue;
    }
    const Tetrodotoxin::Source::Layout& parameters = callable->get_parameters();
    Count parameter_start = parameters.get_name(0).visit(
        []() { return Count(0); },
        [](Core::View::Bytes name) {
          return name == "self"_view ? Count(1) : Count(0);
        });
    Count parameter_count = parameters.get_size() - parameter_start;
    Count mapping_count = parameter_count == arguments.get_size()
                              ? parameter_count
                              : (parameter_count == 1 ? Count(1) : Count(0));
    for (Count index = 0; index < mapping_count; index++) {
      Count offset = arguments[index];
      if (source[offset] == '.') {
        continue;
      }
      auto parameter_name = parameters.get_name(parameter_start + index);
      if (!parameter_name || parameter_name->is_empty()) {
        continue;
      }
      auto position = encoding.locate(source, offset);
      if (!position || position->is_before(start) ||
          !position->is_before(end)) {
        continue;
      }

      Memory::Managed::Bytes label(arena, "."_view);
      label.concat(*parameter_name);
      label.concat(" ="_view);
      hints.insert(
          Serialization::Json::Blueprint{
            {
              {"position"_view,
               {
                 {"line"_view, position->get_line()},
                 {"character"_view, position->get_character()},
               }},
              {"label"_view, label.get_view()},
              {"kind"_view, S64(2)},
              {"paddingRight"_view, True},
            }}.construct(arena));
    }
  }

  return Serialization::Json::Node(hints.get_view());
}
