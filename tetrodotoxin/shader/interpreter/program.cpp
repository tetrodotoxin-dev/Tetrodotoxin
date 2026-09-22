// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/interpreter/program.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/import.hpp"
#include "tetrodotoxin/language/parser/type_reference.hpp"
#include "tetrodotoxin/render/language/attributes.hpp"
#include "tetrodotoxin/shader/interpreter/bridge.hpp"
#include "tetrodotoxin/shader/interpreter/stage.hpp"
#include "tetrodotoxin/shader/interpreter/uniform.hpp"
#include "tetrodotoxin/shader/interpreter/value.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

static auto begins_uniform(const Cursor& cursor) -> Bool {
  S64 offset = 0;
  Code::Type visibility = cursor.peek(offset).get_code().get_type();
  BAIL_IF(
      visibility != Code::Type::Public && visibility != Code::Type::Private &&
      visibility != Code::Type::Expose);
  offset++;
  while (cursor.peek(offset).get_code().is_evaluation_modifier()) {
    offset++;
  }
  BAIL_IF(
      cursor.peek(offset).get_code() != Code::Type::Addressable &&
      cursor.peek(offset).get_code() != Code::Type::Type);
  offset++;
  BAIL_IF(cursor.peek(offset).get_code() != Code::Type::Define);
  offset++;
  return cursor.peek(offset).caculate_text(cursor.get_source_text()) ==
         "uniform"_view;
}

auto Interpreter::Program::parse(
    Shader::Language::Monograph& monograph,
    Cursor& cursor,
    const Tetrodotoxin::Source::Documentation& documentation) -> Bool {
  Token relationship = cursor.require(
      Code::Type::Addressable,
      "Shader sources begin with `implements source(...)`."_view);
  BAIL_IF(!relationship);
  if (relationship.caculate_text(cursor.get_source_text()) !=
      "implements"_view) {
    cursor.create_token_error(
        relationship,
        "Shader sources begin with `implements source(...)`."_view);
    return False;
  }
  Option<Tetrodotoxin::Language::TypeReference> contract;
  Anchor relationship_anchor = Anchor::create(Span());
  if (cursor.matches(Code::Type::Source) ||
      cursor.matches(Code::Type::Package)) {
    auto import = Tetrodotoxin::Language::Parser::Import::parse_expression(
        cursor, documentation, "Pipeline"_view,
        Tetrodotoxin::Language::Visibility::Private, relationship);
    BAIL_IF(
        !import ||
        !monograph.retain_import(*import, cursor.get_associations()));
    contract = Tetrodotoxin::Language::TypeReference::create(
        cursor.get_arena().proxy("Pipeline"_view),
        import->get_expression_anchor());
    relationship_anchor = import->get_declaration_anchor();
  } else {
    contract = Tetrodotoxin::Language::Parser::TypeReference::parse(cursor);
    BAIL_IF(!contract);
    Token closing = cursor.require(
        Code::Type::EndStatement,
        "Shader `implements` relationships require one trailing `;`."_view);
    BAIL_IF(!closing);
    relationship_anchor =
        Anchor::create(relationship, Span(relationship, closing));
  }

  auto& definition = Tetrodotoxin::Language::Definition::create_synthetic(
      cursor.get_arena(), documentation, monograph.edit_library().get_source(),
      "Program"_view, Tetrodotoxin::Language::Visibility::Private,
      relationship_anchor);
  auto& program = Shader::Language::Program::create_authored(
      cursor.get_arena(), definition, *contract, monograph);
  BAIL_IF(!program.initialize_runtime_surface());
  cursor.get_associations().create(
      relationship_anchor, program.get_parameters());
  cursor.get_associations().create(
      relationship_anchor, program.get_instance());

  // Program enters the real Library child before its body is interpreted. Its
  // generated runtime Types and authored Stage bodies then share the ordinary
  // Library completion barriers.
  BAIL_IF(!monograph.edit_library().get_source().retain_definition(
      program, Library::Language::Types::Composite::Category::Type, False));
  BAIL_IF(!monograph.retain_program(program));

  while (!cursor.matches(Code::Type::Terminal)) {
    const Tetrodotoxin::Source::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    if (Interpreter::Stage::is_next(cursor)) {
      if (!Interpreter::Stage::parse(program, cursor, documentation)) {
        cursor.recover_to_scoped_statement();
      }
      continue;
    }

    Bool uniform = begins_uniform(cursor);
    Abstract& host = uniform ? static_cast<Abstract&>(program.edit_parameters())
                             : static_cast<Abstract&>(program);
    auto member_definition =
        Tetrodotoxin::Language::Definition::parse(cursor, documentation, host);
    Bool parsed = False;
    if (member_definition) {
      Token member_qualifier = member_definition->get_authored().get_qualifier();
      View::Bytes qualifier_name =
          member_qualifier.caculate_text(cursor.get_source_text());
      if (qualifier_name == "bridge"_view) {
        parsed =
            Interpreter::Bridge::parse(monograph, cursor, *member_definition);
      } else if (qualifier_name == "uniform"_view) {
        parsed =
            Interpreter::Uniform::parse(program, cursor, *member_definition);
      } else if (Interpreter::Value::matches(*member_definition, cursor)) {
        parsed = Interpreter::Value::parse(program, cursor, *member_definition);
      } else {
        auto report = cursor.create_report(member_definition->get_authored().get_anchor());
        report
            << "Shader Programs author only Stage bodies, storage values, uniforms, and Bridges, not `"_view
            << qualifier_name << "`."_view;
      }
    }
    if (!parsed) {
      cursor.recover_to_scoped_statement();
    }
  }
  program.complete_authored_body();
  return True;
}
