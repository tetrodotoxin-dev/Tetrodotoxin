// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/dialect.hpp"

#include "tetrodotoxin/source/documentation.hpp"


#include "perimortem/memory/managed/vector.hpp"


#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/import.hpp"
#include "tetrodotoxin/library/interpreter/member.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/source/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

static auto parse_quoted(Cursor& cursor) -> Option<View::Bytes> {
  Token token = cursor.require(
      Code::Type::String,
      "Package coordinates require quoted String values."_view);
  BAIL_IF(!token);
  View::Bytes text = token.caculate_text(cursor.get_source_text());
  if (!Lexicon::validate(Code::Type::String, text)) {
    cursor.create_token_error(
        token, "Package coordinate String is unterminated."_view);
    return {};
  }
  return text.slice(1, text.get_size() - 2);
}

static auto parse_named(Cursor& cursor, View::Bytes expected)
    -> Option<View::Bytes> {
  BAIL_IF(!cursor.require(
      Code::Type::AddressOp,
      "Package coordinate arguments require a leading `.`."_view));
  Token name = cursor.require(
      Code::Type::Addressable,
      "Package coordinate arguments require a named slot."_view);
  BAIL_IF(!name);
  if (name.caculate_text(cursor.get_source_text()) != expected) {
    cursor.create_token_error(
        name, "Package coordinate argument is out of order."_view);
    return {};
  }
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "Package coordinate named arguments require `=`."_view));
  return parse_quoted(cursor);
}

static auto parse_coordinate(
    Cursor& cursor,
    View::Bytes& identity,
    Perimortem::System::Version& version) -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::Package,
      "Package sources require `package(.name = ..., .version = ...);`."_view));
  BAIL_IF(!cursor.require(
      Code::Type::PackingStart,
      "Package coordinates require `(` before their arguments."_view));
  auto name = parse_named(cursor, "name"_view);
  BAIL_IF(!name);
  BAIL_IF(!cursor.require(
      Code::Type::PackingOp,
      "Package coordinates require both `name` and `version`."_view));
  auto version_text = parse_named(cursor, "version"_view);
  BAIL_IF(!version_text);
  BAIL_IF(!cursor.require(
      Code::Type::PackingEnd,
      "Package coordinates require `)` after their arguments."_view));
  BAIL_IF(!cursor.require(
      Code::Type::EndStatement,
      "Package coordinates require one terminating `;`."_view));

  version = Perimortem::System::Version::parse(*version_text);
  if (name->is_empty() || version.is_null()) {
    cursor.create_token_error(
        "Package coordinates require a nonempty name and canonical Major.Minor version."_view);
    return False;
  }
  identity = cursor.get_arena().proxy(*name);
  return True;
}

auto Package::Dialect::interpret(
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  Allocator::Arena& transaction = cursor.get_arena();
  View::Bytes identity;
  Perimortem::System::Version version;
  BAIL_IF(!parse_coordinate(cursor, identity, version));

  Managed::Vector<Tetrodotoxin::Language::Import::Description> imports(
      transaction);
  while (Tetrodotoxin::Language::Parser::Import::is_next(cursor)) {
    const Tetrodotoxin::Source::Documentation& import_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto import = Tetrodotoxin::Language::Parser::Import::parse(
        cursor, import_documentation);
    if (import) {
      imports.insert(*import);
    } else {
      cursor.recover_to_statement();
    }
  }

  auto& monograph = Language::Monograph::create_authored(
      transaction, *this, documentation, source_anchor, identity, version,
      context, library);
  auto& root = monograph.edit_library().get_source();
  while (!cursor.matches(Code::Type::Terminal)) {
    const Tetrodotoxin::Source::Documentation& declaration_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto definition = Tetrodotoxin::Language::Definition::parse(
        cursor, declaration_documentation, root);
    if (!definition) {
      cursor.recover_to_statement();
      continue;
    }
    auto member = Library::Interpreter::Member::parse(cursor, *definition);
    if (!member || member->get_category() !=
                       Library::Language::Types::Composite::Category::Type) {
      cursor.create_expression_error(
          definition->get_authored().get_anchor(),
          "Package sources contain only Library Type definitions."_view,
          "Import source or Package roots with Alias declarations, then publish Types or namespaces."_view);
      cursor.recover_to_statement();
      continue;
    }
    root.retain_authored_definition(
        member->get_semantic(), *definition, member->get_category(), cursor);
    if (member->needs_recovery()) {
      cursor.recover_to_statement();
    }
  }
  for (const Tetrodotoxin::Language::Import::Description& import :
       imports.get_view()) {
    if (!monograph.retain_import(import, cursor.get_associations())) {
      cursor.create_expression_error(
          import.get_declaration_anchor(),
          "Source repeats one local Import Type name."_view,
          "Give each imported source or Package one distinct local name."_view);
    }
  }
  return monograph;
}
