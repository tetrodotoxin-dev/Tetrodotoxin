// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/interpreter/value.hpp"

#include "tetrodotoxin/library/interpreter/member.hpp"
#include "tetrodotoxin/library/interpreter/pack.hpp"
#include "tetrodotoxin/library/interpreter/type_reference.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/render/language/attributes.hpp"
#include "tetrodotoxin/shader/language/binding.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

auto Interpreter::Value::matches(
    const Tetrodotoxin::Language::Definition& definition,
    const Cursor& cursor) -> Bool {
  Code::Type qualifier = definition.get_authored().get_qualifier().get_code().get_type();
  if (qualifier == Code::Type::Type || qualifier == Code::Type::Assign) {
    return True;
  }
  View::Bytes name =
      definition.get_authored().get_qualifier().caculate_text(cursor.get_source_text());
  return name == "push"_view || name == "resource"_view;
}

static auto qualifier_name(
    const Tetrodotoxin::Language::Definition& definition,
    Cursor& cursor) -> View::Bytes {
  return definition.get_authored().get_qualifier().caculate_text(cursor.get_source_text());
}

static auto retain_value(
    Shader::Language::Program& program,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition,
    Library::Language::Field& field,
    Render::Language::Binding::Kind kind,
    Render::Language::Binding::Access access,
    Option<Library::Language::TypeReference> runtime_type,
    Bool accepted) -> Bool {
  Bool retained = program.retain_authored_definition(
      field, definition,
      Library::Language::Types::Composite::Category::Addressable, cursor);
  Option<Library::Language::Field&> instance_field;
  if (retained && kind == Render::Language::Binding::Kind::Resource) {
    BAIL_IF(!runtime_type);
    instance_field = program.retain_instance_resource(field, *runtime_type);
    retained = Bool(instance_field);
  }
  if (retained) {
    program.retain_shader_binding(field, kind, access, instance_field);
  }
  return retained && accepted;
}

static auto parse_library_value(
    Shader::Language::Program& program,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  auto member = Library::Interpreter::Member::parse(cursor, definition);
  BAIL_IF(!member);
  auto field = member->get_semantic().select<Library::Language::Field>();
  if (!field) {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Shader value definitions create one Library Field."_view);
    return False;
  }

  Bool attributes_valid = Render::Language::Attributes::validate(
      cursor, definition.get_attributes(),
      Render::Language::Attributes::Placement::Value);
  Render::Language::Binding::Kind kind =
      field->get_writability() == Library::Language::Writability::Constant
          ? Render::Language::Binding::Kind::Constant
          : Render::Language::Binding::Kind::Value;
  Bool retained = retain_value(
      program, cursor, definition, *field, kind,
      Render::Language::Binding::Access::None, {}, member->is_accepted());
  if (member->needs_recovery()) {
    cursor.recover_to_scoped_statement();
  }
  return attributes_valid && retained;
}

static auto parse_shader_value(
    Shader::Language::Program& program,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition,
    View::Bytes qualifier) -> Bool {
  if (definition.get_authored().get_name().get_code() != Code::Type::Addressable) {
    cursor.create_token_error(
        definition.get_authored().get_name(),
        "Shader storage definitions use one addressable name."_view);
    return False;
  }
  if (!definition.get_authored().get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_authored().get_modifiers().get_data()[0],
        "Shader resource and push definitions do not accept evaluation modifiers."_view);
    return False;
  }
  Token qualifier_token = cursor.consume();
  Render::Language::Binding::Access access =
      Render::Language::Binding::Access::None;
  if (qualifier == "resource"_view) {
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
          "Shader resources require `read`, `write`, or both capabilities."_view);
      return False;
    }
    access = read && write ? Render::Language::Binding::Access::ReadWrite
             : read        ? Render::Language::Binding::Access::Read
                           : Render::Language::Binding::Access::Write;
  }
  auto type = Library::Interpreter::TypeReference::parse(program, cursor);
  if (!type) {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Shader storage requires one complete Library Type reference."_view);
    return False;
  }
  Option<Library::Language::TypeReference> runtime_type;
  if (qualifier == "resource"_view) {
    runtime_type = *type;
    BAIL_IF(!cursor.require(
        Code::Type::CallOp,
        "Shader resource requires `->` between its runtime and GPU Types."_view));
    type = Library::Interpreter::TypeReference::parse(program, cursor);
    BAIL_IF(!type);
  }
  Option<Library::Language::Model::Pack&> initializer;
  if (cursor.matches(Code::Type::Assign)) {
    if (qualifier == "resource"_view) {
      cursor.create_token_error(
          "Shader resources are configured through their generated Material Field."_view);
      return False;
    }
    cursor.consume();
    initializer = Library::Interpreter::Pack::parse(program, cursor);
    BAIL_IF(!initializer);
  }
  Token closing = cursor.require(
      Code::Type::EndStatement,
      "Shader storage definitions require one trailing `;`."_view);
  BAIL_IF(!closing);

  Render::Language::Binding::Kind kind =
      qualifier == "push"_view ? Render::Language::Binding::Kind::Push
                               : Render::Language::Binding::Kind::Resource;
  Render::Language::Attributes::Placement placement =
      qualifier == "push"_view
          ? Render::Language::Attributes::Placement::Push
          : Render::Language::Attributes::Placement::Resource;
  Bool attributes_valid = Render::Language::Attributes::validate(
      cursor, definition.get_attributes(), placement);
  auto& field = Library::Language::Field::create_authored(
      cursor.get_arena(), definition, Library::Language::Writability::Full,
      *type, initializer);
  auto& authored = definition.get_authored();
  authored.set_anchor(Anchor::create(
      qualifier_token,
      Span(authored.get_anchor().get_span().get_start(), closing)));
  Bool retained = retain_value(
      program, cursor, definition, field, kind, access, runtime_type,
      True);
  return attributes_valid && retained;
}

auto Interpreter::Value::parse(
    Shader::Language::Program& program,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  View::Bytes qualifier = qualifier_name(definition, cursor);
  return qualifier == "push"_view || qualifier == "resource"_view
             ? parse_shader_value(program, cursor, definition, qualifier)
             : parse_library_value(program, cursor, definition);
}
