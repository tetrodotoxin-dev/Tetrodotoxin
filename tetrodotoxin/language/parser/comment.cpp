// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/parser/comment.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/source/lexical/lexicon.hpp"
#include "tetrodotoxin/source/lexical/token.hpp"
#include "tetrodotoxin/source/documentations/block.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Language;

// Documentation comments support both `// text` and `//text` however the
// canonical form is always `// text`. This function extracts the line text
// regardless of form which simplifies consumers.
//
// Multiple spaces past the first are preserved.
static constexpr auto comment_line(Token comment, View::Bytes source)
    -> View::Bytes {
  View::Bytes line = comment.caculate_text(source);
  const Count marker_size =
      Lexicon::get_spelling(Code::Type::Comment).get_size();
  line = line.slice(marker_size, line.get_size() - marker_size);
  if (!line.is_empty() && line[0] == ' ') {
    return line.slice(1, line.get_size() - 1);
  }

  return line;
}

auto Parser::Comment::parse(Cursor& cursor) -> const Tetrodotoxin::Source::Documentation& {
  // Absence is valid for nested parser positions. Document parsers enforce
  // their required opening comment before delegating here.
  if (!cursor.get_code().is_comment()) {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }

  Managed::Vector<View::Bytes> lines(cursor.get_arena());
  while (cursor.get_code().is_comment()) {
    Token comment = cursor.consume();
    if (comment.get_code() == Code::Type::RawComment) {
      continue;
    }
    lines.insert(comment_line(comment, cursor.get_source_text()));
  }

  if (lines.is_empty()) {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }

  // Block and its line index share the Source arena. Each line still borrows
  // the tokenizer's source bytes so formatters and other tools can examine the
  // block just as it was source authored.
  const auto& documentation =
      cursor.get_arena().construct<Tetrodotoxin::Source::Documentations::Block>(
          lines.get_view());
  return documentation;
}
