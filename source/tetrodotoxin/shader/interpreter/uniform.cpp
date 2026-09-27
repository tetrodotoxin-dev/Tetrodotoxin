// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/interpreter/uniform.hpp"

#include "tetrodotoxin/library/interpreter/pack.hpp"
#include "tetrodotoxin/library/interpreter/type_reference.hpp"
#include "tetrodotoxin/library/language/field.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

auto Interpreter::Uniform::parse(
    Shader::Language::Program& program,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  if (definition.get_authored().get_name().get_code() != Code::Type::Addressable) {
    cursor.create_token_error(
        definition.get_authored().get_name(),
        "Shader uniforms use one addressable name."_view);
    return False;
  }
  if (definition.get_visibility() !=
      Tetrodotoxin::Language::Visibility::Public) {
    cursor.create_token_error(
        definition.get_authored().get_visibility(),
        "Shader uniforms use public visibility."_view);
    return False;
  }
  if (!definition.get_authored().get_modifiers().is_empty() ||
      !definition.get_attributes().is_empty()) {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Shader uniforms do not accept modifiers or Attributes."_view);
    return False;
  }

  Token qualifier = cursor.consume();
  auto type = Library::Interpreter::TypeReference::parse(
      program.edit_parameters(), cursor);
  BAIL_IF(!type);
  Option<Library::Language::Model::Pack&> initializer;
  if (cursor.matches(Code::Type::Assign)) {
    cursor.consume();
    initializer =
        Library::Interpreter::Pack::parse(program.edit_parameters(), cursor);
    BAIL_IF(!initializer);
  }
  Token closing = cursor.require(
      Code::Type::EndStatement,
      "Shader uniform declarations require one trailing `;`."_view);
  BAIL_IF(!closing);
  auto& authored = definition.get_authored();
  authored.set_anchor(Anchor::create(
      qualifier, Span(authored.get_anchor().get_span().get_start(), closing)));

  auto& field = Library::Language::Field::create_authored(
      cursor.get_arena(), definition, Library::Language::Writability::Internal,
      *type, initializer);
  BAIL_IF(!program.edit_parameters().retain_authored_definition(
      field, definition,
      Library::Language::Types::Composite::Category::Addressable, cursor));
  program.retain_uniform(field);
  return True;
}
