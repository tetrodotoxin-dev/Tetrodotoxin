// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/interpreter/runtime.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/core/reader/textual.hpp"

#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/source/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

struct RuntimeSettings {
  Option<View::Bytes> title;
  Option<View::Bytes> icon_route;
  Option<const Language::Resource&> icon;
  Option<U32> width;
  Option<U32> height;
  Option<Bool> resizable;
};

static auto require_text(
    Cursor& cursor,
    Code::Type code,
    View::Bytes text,
    View::Bytes message) -> Token {
  if (!cursor.matches(code) || cursor.get_text() != text) {
    cursor.create_token_error(cursor.current(), message);
    return {};
  }

  return cursor.consume();
}

static auto decode_string(Cursor& cursor) -> Option<View::Bytes> {
  Token token = cursor.require(
      Code::Type::String, "App title requires one quoted string."_view);
  BAIL_IF(!token);
  View::Bytes text = token.caculate_text(cursor.get_source_text());
  if (!Lexicon::validate(Code::Type::String, text)) {
    cursor.create_token_error(
        token, "App title requires one closed quoted string."_view);
    return {};
  }
  View::Bytes payload = text.slice(1, text.get_size() - 2);
  Count size = payload.get_size();
  for (Count index = 0; index < payload.get_size(); index++) {
    if (payload[index] == '\\') {
      size--;
      index++;
    }
  }

  auto output = cursor.get_arena().allocate(size);
  Count written = 0;
  for (Count index = 0; index < payload.get_size(); index++) {
    if (payload[index] == '\\') {
      index++;
    }
    output.get_data()[written++] = payload[index];
  }
  return output.get_view();
}

static auto parse_extent(Cursor& cursor, View::Bytes setting) -> Option<U32> {
  Token token = cursor.require(
      Code::Type::Numeric,
      "App window dimensions require unsigned decimal values."_view);
  BAIL_IF(!token);
  View::Bytes text = token.caculate_text(cursor.get_source_text());
  Reader::Textual reader(text);
  U64 value = reader.read_unsigned();
  if (!reader.is_valid() || reader.get_location() != reader.get_size() ||
      value > U32(-1)) {
    auto report = cursor.create_report(Span(token));
    report << "App `"_view << setting
           << "` must fit in one unsigned 32 bit value."_view;
    return {};
  }
  return U32(value);
}

static auto parse_flag(Cursor& cursor) -> Option<Bool> {
  if (!cursor.is_one_of({{Code::Type::True, Code::Type::False}})) {
    cursor.create_token_error(
        "App `resizable` requires `true` or `false`."_view);
    return {};
  }
  return Bool(cursor.consume().get_code() == Code::Type::True);
}

static auto parse_icon(
    Cursor& cursor,
    Abstract& context,
    RuntimeSettings& settings,
    Bool retain) -> Bool {
  Token token = cursor.require(
      Code::Type::Embedded,
      "App `icon` requires one embedded Package Resource."_view);
  BAIL_IF(!token);
  View::Bytes route = token.caculate_text(cursor.get_source_text());
  auto resource =
      context.resolve_concept(route).resolve().select<Language::Resource>();
  if (!resource) {
    cursor.create_expression_error(
        Span(token), "App icon did not resolve to a Package Resource."_view,
        "Use one confined `$[...]` route from this source Package."_view);
    return False;
  }

  if (retain) {
    settings.icon_route = route;
    settings.icon = *resource;
    cursor.get_associations().create(Anchor::create(Span(token)), *resource);
  }
  return True;
}

static auto finish_setting(Cursor& cursor) -> Bool {
  if (cursor.is_one_of({{Code::Type::PackingOp, Code::Type::EndStatement}})) {
    cursor.consume();
    return True;
  }
  cursor.create_token_error(
      "App runtime setting requires a trailing `,` or `;`."_view);
  return False;
}

static auto parse_setting(
    Cursor& cursor,
    Abstract& context,
    RuntimeSettings& settings) -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::AddressOp,
      "App runtime setting requires a leading `.`."_view));
  Token name_token = cursor.require(
      Code::Type::Addressable,
      "App runtime setting requires one name after `.`."_view);
  BAIL_IF(!name_token);
  View::Bytes name = name_token.caculate_text(cursor.get_source_text());
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "App runtime setting requires `=` before its value."_view));

  Bool valid = True;
  if (name == "title"_view) {
    if (settings.title) {
      cursor.create_token_error(name_token, "App `title` is already set."_view);
      valid = False;
    }
    auto value = decode_string(cursor);
    valid &= Bool(value);
    if (value && !settings.title) {
      settings.title = *value;
    }
  } else if (name == "icon"_view) {
    if (settings.icon) {
      cursor.create_token_error(name_token, "App `icon` is already set."_view);
      valid = False;
    }
    valid &= parse_icon(cursor, context, settings, !settings.icon);
  } else if (name == "width"_view || name == "height"_view) {
    Option<U32>& retained =
        name == "width"_view ? settings.width : settings.height;
    if (retained) {
      auto report = cursor.create_report(Span(name_token));
      report << "App `"_view << name << "` is already set."_view;
      valid = False;
    }
    auto value = parse_extent(cursor, name);
    valid &= Bool(value);
    if (value && !retained) {
      retained = *value;
    }
  } else if (name == "resizable"_view) {
    if (settings.resizable) {
      cursor.create_token_error(
          name_token, "App `resizable` is already set."_view);
      valid = False;
    }
    auto value = parse_flag(cursor);
    valid &= Bool(value);
    if (value && !settings.resizable) {
      settings.resizable = *value;
    }
  } else {
    auto report = cursor.create_report(Span(name_token));
    report << "Unknown App runtime setting `"_view << name << "`."_view;
    report.get_hint()
        << "Windowed accepts title, icon, width, height, and resizable."_view;
    cursor.recover_to_scoped_statement();
    return False;
  }

  if (!valid) {
    cursor.recover_to_scoped_statement();
    return False;
  }
  return finish_setting(cursor);
}

auto App::Interpreter::Runtime::parse(
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation,
    Abstract& context) -> Option<App::Language::Runtime&> {
  Token opening = require_text(
      cursor, Code::Type::Addressable, "runtime"_view,
      "App runtime declaration requires `runtime`."_view);
  BAIL_IF(!opening);
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "App runtime declaration requires `=` before its profile."_view));
  Token profile_token = cursor.require(
      Code::Type::Type,
      "App runtime declaration requires Terminal, Headless, or Windowed."_view);
  BAIL_IF(!profile_token);
  View::Bytes profile_name =
      profile_token.caculate_text(cursor.get_source_text());
  App::Language::Runtime::Profile profile;
  if (profile_name == "Terminal"_view) {
    profile = App::Language::Runtime::Profile::Terminal;
  } else if (profile_name == "Headless"_view) {
    profile = App::Language::Runtime::Profile::Headless;
  } else if (profile_name == "Windowed"_view) {
    profile = App::Language::Runtime::Profile::Windowed;
  } else {
    cursor.create_token_error(
        profile_token,
        "App runtime profile must be Terminal, Headless, or Windowed."_view);
    return {};
  }

  RuntimeSettings settings;
  Token closing = profile_token;
  if (cursor.matches(Code::Type::EndStatement)) {
    closing = cursor.consume();
  } else {
    BAIL_IF(!cursor.require(
        Code::Type::ScopeStart,
        "App runtime profile requires `{}` settings or a trailing `;`."_view));
    while (!cursor.matches(Code::Type::ScopeEnd) &&
           !cursor.matches(Code::Type::Terminal)) {
      if (profile != App::Language::Runtime::Profile::Windowed) {
        cursor.create_token_error(
            "Only Windowed accepts runtime settings."_view);
        cursor.recover_to_scoped_statement();
        continue;
      }
      parse_setting(cursor, context, settings);
    }
    closing = cursor.require(
        Code::Type::ScopeEnd,
        "App runtime profile requires a closing `}`."_view);
    BAIL_IF(!closing);
  }

  Anchor anchor = Anchor::create(opening, Span(opening, closing));
  App::Language::Runtime* runtime = nullptr;
  if (profile == App::Language::Runtime::Profile::Windowed) {
    runtime = &App::Language::Runtime::create_windowed(
        cursor.get_arena(), documentation, anchor, settings.title,
        settings.icon_route, settings.icon, settings.width, settings.height,
        settings.resizable);
  } else {
    runtime = &App::Language::Runtime::create_authored(
        cursor.get_arena(), documentation, profile, anchor);
  }
  cursor.get_associations().create(runtime->get_anchor(), *runtime);
  return *runtime;
}
