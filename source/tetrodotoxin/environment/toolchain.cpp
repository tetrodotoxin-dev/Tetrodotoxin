// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/toolchain.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/dialect.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Language;
using namespace Tetrodotoxin::Environment;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;

static auto parse_dialect(const Toolchain& toolchain, Cursor& cursor)
    -> Option<Dialect&> {
  // Parse the actual Dialect
  const Token declaration = cursor.current();
  const auto name = Parser::Dialect::parse(cursor);
  if (name.is_empty()) {
    return {};
  }

  auto dialect = toolchain.find(name);
  if (!dialect) {
    auto report = cursor.create_report(Span(declaration));
    report << "Unknown dialect "_view << name;
    report.get_hint()
        << "Install the requested Dialect before processing this source."_view;
  }

  return dialect;
}

Toolchain::Toolchain() : dialects(arena), sources(arena) {}

Toolchain::~Toolchain() {
  for (Count index = sources.get_size(); index > 0; index--) {
    sources[index - 1].get().~Monograph();
  }
}

auto Toolchain::process(View::Bytes source, Errors& errors)
    -> Option<Monograph&> {
  // In case we are a toolchain hosted in some weird one off context we can't
  // assume that `source` is a stable value that will outlive the toolchain so
  // we'll need to manually proxy it. It's not a _huge waste_ but uncessary
  // proxies are always a red flag for lifetime semantics.
  const auto path = arena.proxy(source);
  auto contents = Perimortem::System::File::read(arena, path);
  if (!contents) {
    Errors::Report report(errors, path, {}, Anchor::create(Span()));
    report << "Could not read source "_view << path;
    return {};
  }

  // Tokenize the file as TTX (Toolchain Text Extension) format and parse the
  // required header (comment + Dialect decleration).
  Tokenizer tokenizer(arena, *contents, path);
  Associations associations(arena);
  Cursor cursor(tokenizer, errors, associations);
  Token opening = cursor.current();
  const Tetrodotoxin::Source::Documentation& documentation = Language::Parser::Comment::parse(cursor);
  if (documentation.is_empty()) {
    Errors::Report report(errors, path, {}, Anchor::create(Span()));
    report << "Could not read source "_view << path;
    cursor.create_token_error(
        opening, "Source TTX requires a documentation header."_view);
    return {};
  }

  // Parse the actual Dialect requested by the file and see if we have it
  // installed. If not then we can't interpret the rest of the file.
  const Token declaration = cursor.current();
  auto dialect = parse_dialect(*this, cursor);
  if (!dialect) {
    return {};
  }

  // The installed Dialect is the only outer authority at bootstrap. Its
  // interpreter receives the body unchanged and creates its own root.
  const auto anchor =
      Anchor::create(declaration, Span(opening, cursor.peek(-1)));
  const Count error_count = errors.get_size();
  Option<Monograph&> result =
      dialect->interpret(cursor, documentation, anchor, *dialect);
  if (result) {
    sources.insert(*result);
  } else if (errors.get_size() == error_count) {
    cursor.create_error(
        "The selected Dialect could not interpret this source."_view);
  }

  return result;
}

auto Toolchain::contains_name(View::Bytes name) const -> Bool {
  return dialects.get_view().contains(
      [&](const Reference<Language::Dialect>& dialect) {
        return dialect.get().get_name() == name;
      });
}

auto Toolchain::find(View::Bytes name) const -> Option<Language::Dialect&> {
  for (const Reference<Language::Dialect>& dialect : dialects.get_view()) {
    if (dialect.get().get_name() == name) {
      return dialect.get();
    }
  }

  return {};
}
