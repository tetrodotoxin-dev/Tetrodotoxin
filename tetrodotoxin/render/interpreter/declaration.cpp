// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/interpreter/declaration.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/type_reference.hpp"
#include "tetrodotoxin/render/interpreter/layout.hpp"
#include "tetrodotoxin/render/language/alias.hpp"
#include "tetrodotoxin/render/language/attributes.hpp"
#include "tetrodotoxin/render/language/binding.hpp"
#include "tetrodotoxin/render/language/stage.hpp"
#include "tetrodotoxin/render/language/structure.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Render;

enum class DeclarationCategory : U8 {
  Addressable,
  Callable,
  Type,
};

static auto retain(
    Abstract& host,
    Abstract& declaration,
    DeclarationCategory category,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  auto monograph = host.select<Language::Monograph>();
  if (monograph) {
    switch (category) {
    case DeclarationCategory::Addressable:
      return monograph->retain_addressable(declaration, visibility);
    case DeclarationCategory::Callable:
      return monograph->retain_callable(declaration, visibility);
    case DeclarationCategory::Type:
      return monograph->retain_type(declaration, visibility);
    }
  }
  auto structure = host.select<Language::Structure>();
  BAIL_IF(!structure);
  switch (category) {
  case DeclarationCategory::Addressable:
    return structure->retain_addressable(declaration, visibility);
  case DeclarationCategory::Callable:
    return structure->retain_callable(declaration, visibility);
  case DeclarationCategory::Type:
    return structure->retain_type(declaration, visibility);
  }
  return False;
}

static auto complete(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition,
    Token closing,
    Abstract& declaration) -> Bool {
  BAIL_IF(!closing);
  auto& authored = definition.get_authored();
  authored.set_anchor(Anchor::create(
      authored.get_qualifier(),
      Span(authored.get_anchor().get_span().get_start(), closing)));
  cursor.get_associations().create(Anchor::create(Span(authored.get_name())), declaration);
  return True;
}

static auto has_supported_name(
    Cursor& cursor,
    const Tetrodotoxin::Language::Definition& definition,
    Code::Type expected,
    View::Bytes message) -> Bool {
  if (definition.get_authored().get_name().get_code() == expected) {
    return True;
  }
  cursor.create_token_error(definition.get_authored().get_name(), message);
  return False;
}

static auto has_no_modifiers(
    Cursor& cursor,
    const Tetrodotoxin::Language::Definition& definition,
    View::Bytes message) -> Bool {
  if (definition.get_authored().get_modifiers().is_empty()) {
    return True;
  }
  cursor.create_token_error(definition.get_authored().get_modifiers().get_data()[0], message);
  return False;
}

static auto parse_stage(
    Abstract& host,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  BAIL_IF(!has_supported_name(
      cursor, definition, Code::Type::Addressable,
      "Pipeline Stages use one addressable name."_view));
  BAIL_IF(!has_no_modifiers(
      cursor, definition,
      "Pipeline Stages do not accept evaluation modifiers."_view));
  cursor.consume();

  auto parameters = Interpreter::Layout::parse(cursor, True);
  BAIL_IF(!parameters);
  BAIL_IF(!cursor.require(
      Code::Type::CallOp,
      "Pipeline Stage requires `->` between its Layouts."_view));
  auto results = Interpreter::Layout::parse(cursor, False);
  BAIL_IF(!results);
  Token closing = cursor.require(
      Code::Type::EndStatement,
      "Pipeline Stage requires one trailing `;`."_view);
  BAIL_IF(!closing);

  auto& stage = Language::Stage::create(
      cursor.get_arena(), definition, *parameters, *results);
  BAIL_IF(!retain(
      host, stage, DeclarationCategory::Callable, definition.get_visibility()));
  return complete(cursor, definition, closing, stage);
}

static auto parse_binding(
    Abstract& host,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition,
    View::Bytes qualifier) -> Bool {
  BAIL_IF(!has_supported_name(
      cursor, definition, Code::Type::Addressable,
      "Pipeline values use one addressable name."_view));

  Language::Binding::Kind kind = Language::Binding::Kind::Value;
  Language::Binding::Access access = Language::Binding::Access::None;
  Language::Attributes::Placement placement =
      Language::Attributes::Placement::Value;
  auto modifiers = definition.get_authored().get_modifiers();
  if (qualifier == "push"_view || qualifier == "resource"_view) {
    BAIL_IF(!has_no_modifiers(
        cursor, definition,
        "Pipeline push and resource values do not accept evaluation modifiers."_view));
    cursor.consume();
    if (qualifier == "push"_view) {
      kind = Language::Binding::Kind::Push;
      placement = Language::Attributes::Placement::Push;
    } else {
      kind = Language::Binding::Kind::Resource;
      placement = Language::Attributes::Placement::Resource;
      Bool read = False;
      Bool write = False;
      while (cursor.matches(Code::Type::Addressable)) {
        View::Bytes capability =
            cursor.current().caculate_text(cursor.get_source_text());
        if (capability == "read"_view && !read) {
          read = True;
        } else if (capability == "write"_view && !write) {
          write = True;
        } else {
          break;
        }
        cursor.consume();
      }
      if (!read && !write) {
        cursor.create_token_error(
            "Pipeline resources require `read`, `write`, or both capabilities."_view);
        return False;
      }
      access = read && write ? Language::Binding::Access::ReadWrite
               : read        ? Language::Binding::Access::Read
                             : Language::Binding::Access::Write;
    }
  } else if (!modifiers.is_empty()) {
    if (modifiers.get_size() != 1 ||
        modifiers.get_data()[0].get_code() != Code::Type::Const) {
      cursor.create_token_error(
          modifiers.get_data()[0],
          "Pipeline values accept only `const` evaluation."_view);
      return False;
    }
    kind = Language::Binding::Kind::Constant;
  }

  auto type = Tetrodotoxin::Language::Parser::TypeReference::parse(cursor);
  BAIL_IF(!type);
  Token closing = cursor.require(
      Code::Type::EndStatement,
      "Pipeline value declaration requires one trailing `;`."_view);
  BAIL_IF(!closing);
  BAIL_IF(!Language::Attributes::validate(
      cursor, definition.get_attributes(), placement));

  auto& binding = Language::Binding::create_authored(
      cursor.get_arena(), definition, kind, *type, access);
  BAIL_IF(!retain(
      host, binding, DeclarationCategory::Addressable,
      definition.get_visibility()));
  auto structure = host.select<Language::Structure>();
  if (structure && kind == Language::Binding::Kind::Value) {
    structure->retain_instance(binding);
  }
  return complete(cursor, definition, closing, binding);
}

static auto parse_alias(
    Abstract& host,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  BAIL_IF(!has_supported_name(
      cursor, definition, Code::Type::Type,
      "Pipeline Aliases use one Type name."_view));
  BAIL_IF(!has_no_modifiers(
      cursor, definition,
      "Pipeline Aliases do not accept evaluation modifiers."_view));
  cursor.consume();
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "Pipeline Alias requires `=` before its target."_view));
  auto target = Tetrodotoxin::Language::Parser::TypeReference::parse(cursor);
  BAIL_IF(!target);
  Token closing = cursor.require(
      Code::Type::EndStatement,
      "Pipeline Alias requires one trailing `;`."_view);
  BAIL_IF(!closing);
  auto& alias =
      Language::Alias::create(cursor.get_arena(), definition, *target);
  BAIL_IF(!retain(
      host, alias, DeclarationCategory::Type, definition.get_visibility()));
  return complete(cursor, definition, closing, alias);
}

static auto parse_structure(
    Abstract& host,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  BAIL_IF(!has_supported_name(
      cursor, definition, Code::Type::Type,
      "Pipeline Structures use one Type name."_view));
  BAIL_IF(!has_no_modifiers(
      cursor, definition,
      "Pipeline Structures do not accept evaluation modifiers."_view));
  cursor.consume();
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "Pipeline Structure requires one `{}` body."_view));
  auto& structure = Language::Structure::create(cursor.get_arena(), definition);
  BAIL_IF(!retain(
      host, structure, DeclarationCategory::Type, definition.get_visibility()));

  // A nested Structure uses the same Definition envelope as its source. The
  // concrete Structure remains the lookup owner, which preserves its privacy
  // boundary without introducing a general language Scope object.
  while (!cursor.matches(Code::Type::ScopeEnd) &&
         !cursor.matches(Code::Type::Terminal)) {
    const Tetrodotoxin::Source::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    if (!Interpreter::Declaration::parse(structure, cursor, documentation)) {
      cursor.recover_to_statement();
    }
  }
  Token closing = cursor.require(
      Code::Type::ScopeEnd,
      "Pipeline Structure requires one closing `}`."_view);
  BAIL_IF(!closing);
  return complete(cursor, definition, closing, structure);
}

auto Interpreter::Declaration::parse(
    Abstract& host,
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation) -> Bool {
  auto definition =
      Tetrodotoxin::Language::Definition::parse(cursor, documentation, host);
  BAIL_IF(!definition);

  Token qualifier = definition->get_authored().get_qualifier();
  View::Bytes name = qualifier.caculate_text(cursor.get_source_text());
  if (name == "stage"_view) {
    return parse_stage(host, cursor, *definition);
  }
  if (name == "push"_view || name == "resource"_view) {
    return parse_binding(host, cursor, *definition, name);
  }
  if (qualifier.get_code() == Code::Type::Struct) {
    return parse_structure(host, cursor, *definition);
  }
  if (qualifier.get_code() == Code::Type::Alias) {
    return parse_alias(host, cursor, *definition);
  }
  return parse_binding(host, cursor, *definition, name);
}
