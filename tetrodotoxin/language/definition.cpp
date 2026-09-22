// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/definition.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/source/documentations/merged.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto Language::Definition::parse(
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation,
    Abstract& host,
    Option<View::Vector<Attribute>> supplied_attributes)
    -> Option<Definition&> {
  // Definition owns the source envelope shared by every concrete declaration.
  // Dispatch has already committed to this declaration grammar. The semantic
  // owner is constructed only after its complete common prefix is accepted.
  Token opening = cursor.current();
  Managed::Vector<Language::Attribute> parsed_attributes(cursor.get_arena());
  const Tetrodotoxin::Source::Documentation* retained_documentation = &documentation;
  View::Vector<Language::Attribute> retained_attributes;
  if (supplied_attributes) {
    // An embedding interpreter may discover the outer declaration only after
    // consuming Attributes. An engaged empty view records that decision just
    // as clearly as a populated one, so Definition leaves the Cursor alone.
    retained_attributes = *supplied_attributes;
  } else {
    while (cursor.matches(Code::Type::Attribute) ||
           cursor.get_code().is_comment()) {
      if (cursor.matches(Code::Type::Attribute)) {
        Count error_count = cursor.get_error_count();
        auto parsed = Language::Attribute::parse(cursor);
        BAIL_IF(parsed.is_empty() && cursor.get_error_count() != error_count);
        for (const Language::Attribute& attribute : parsed) {
          parsed_attributes.insert(attribute);
        }
        continue;
      }

      const Tetrodotoxin::Source::Documentation& continued = Language::Parser::Comment::parse(cursor);
      if (continued.is_empty()) {
        continue;
      }
      if (retained_documentation->is_empty()) {
        retained_documentation = &continued;
      } else {
        retained_documentation =
            &cursor.get_arena().construct<Tetrodotoxin::Source::Documentations::Merged>(
                *retained_documentation, continued);
      }
    }
    retained_attributes = parsed_attributes.get_view();
  }

  Token visibility_token = cursor.current();
  Visibility visibility = Visibility::Private;
  switch (visibility_token.get_code().get_type()) {
  case Code::Type::Public:
    visibility = Visibility::Public;
    break;
  case Code::Type::Private:
    visibility = Visibility::Private;
    break;
  case Code::Type::Expose:
    visibility = Visibility::Exposed;
    break;
  default:
    cursor.create_token_error(
        "Definitions require one authored visibility before their name."_view);
    return {};
  }
  cursor.consume();

  Managed::Vector<Token> modifiers(cursor.get_arena());
  while (cursor.get_code().is_evaluation_modifier()) {
    modifiers.insert(cursor.consume());
  }
  if (cursor.get_code().is_publication_modifier()) {
    cursor.create_token_error(
        cursor.current(),
        "Definitions retain exactly one authored visibility."_view);
    return {};
  }

  Token name_token = cursor.current();
  if (name_token.get_code() != Code::Type::Addressable &&
      name_token.get_code() != Code::Type::Type) {
    cursor.create_token_error(
        "Definitions require one authored name after their modifiers."_view);
    return {};
  }
  cursor.consume();

  BAIL_IF(!cursor.require(
      Code::Type::Define,
      "Definitions require `:` between their name and qualifier."_view));

  Token qualifier = cursor.current();
  if (qualifier.get_code() == Code::Type::Terminal ||
      qualifier.get_code() == Code::Type::EndStatement ||
      qualifier.get_code() == Code::Type::ScopeEnd) {
    cursor.create_token_error(
        "Definitions require one qualifier after `:`."_view);
    return {};
  }

  View::Bytes name = name_token.caculate_text(cursor.get_source_text());
  // Documentation, Attributes, Tokens, and the name remain source backed. The
  // Cursor Arena gives the Definition exactly the lifetime of its candidate
  // semantic graph without copying those facts into another owner.
  Definition& definition =
      cursor.get_arena().construct_from<Definition>([&]() -> Definition {
        return Definition(
            *retained_documentation, retained_attributes, modifiers.get_view(),
            visibility, visibility_token, name, name_token, qualifier, host,
            Anchor::create(name_token, Span(opening, qualifier)));
      });
  return definition;
}

auto Language::Definition::create_synthetic(
    Allocator::Arena& domain,
    const Tetrodotoxin::Source::Documentation& documentation,
    Abstract& host,
    View::Bytes reserved_name,
    Visibility visibility,
    Anchor anchor,
    View::Vector<Attribute> attributes) -> Definition& {
  return domain.construct_from<Definition>([&]() -> Definition {
    return Definition(
        documentation, attributes, {}, visibility, {}, reserved_name, {}, {},
        host, anchor);
  });
}

auto Language::Definition::create_restored(
    Allocator::Arena& domain,
    const Tetrodotoxin::Source::Documentation& documentation,
    Abstract& host,
    View::Vector<Attribute> attributes,
    View::Bytes name,
    Visibility visibility) -> Definition& {
  return domain.construct_from<Definition>([&]() -> Definition {
    return Definition(
        documentation, attributes, {}, visibility, {}, name, {}, {}, host,
        Anchor::create(Span()));
  });
}

auto Language::Definition::create_authored(
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation,
    Abstract& host,
    View::Vector<Attribute> attributes,
    View::Vector<Token> modifiers,
    Visibility visibility,
    Token visibility_token,
    View::Bytes name,
    Token name_token,
    Token qualifier,
    Anchor anchor) -> Definition& {
  return cursor.get_arena().construct_from<Definition>([&]() -> Definition {
    return Definition(
        documentation, attributes, modifiers, visibility, visibility_token,
        name, name_token, qualifier, host, anchor);
  });
}

auto Language::Definition::create_authored_prefix(
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation,
    Abstract& host,
    View::Vector<Attribute> attributes,
    View::Vector<Token> modifiers,
    Visibility visibility,
    Token visibility_token,
    View::Bytes name,
    Token name_token,
    Token qualifier,
    Anchor anchor) -> Definition& {
  return cursor.get_arena().construct_from<Definition>([&]() -> Definition {
    return Definition(
        documentation, attributes, modifiers, visibility, visibility_token,
        name, name_token, qualifier, host, anchor);
  });
}
