// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/member.hpp"

#include "tetrodotoxin/library/interpreter/declarations/alias.hpp"
#include "tetrodotoxin/library/interpreter/declarations/field.hpp"
#include "tetrodotoxin/library/interpreter/declarations/function.hpp"
#include "tetrodotoxin/library/interpreter/types/enumeration.hpp"
#include "tetrodotoxin/library/interpreter/types/implemented.hpp"
#include "tetrodotoxin/library/interpreter/types/interface.hpp"
#include "tetrodotoxin/library/interpreter/types/namespace.hpp"
#include "tetrodotoxin/library/interpreter/types/object.hpp"
#include "tetrodotoxin/library/interpreter/types/structure.hpp"
#include "tetrodotoxin/library/language/alias.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Library::Language;

auto Interpreter::Member::parse(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Result> {
  Token qualifier = definition.get_authored().get_qualifier();
  switch (qualifier.get_code().get_type()) {
  case Code::Type::Type:
  case Code::Type::Assign: {
    auto field = Declarations::Field::parse(cursor, definition);
    BAIL_IF(!field);
    return Result(
        field->get_semantic(),
        Language::Types::Composite::Category::Addressable, field->get_state());
  }
  case Code::Type::Alias: {
    auto alias = Declarations::Alias::parse(cursor, definition);
    BAIL_IF(!alias);
    return Result(
        alias->get_semantic(), Language::Types::Composite::Category::Type,
        alias->get_state());
  }
  case Code::Type::Func: {
    auto function = Declarations::Function::parse(cursor, definition);
    BAIL_IF(!function);
    return Result(
        function->get_semantic(),
        Language::Types::Composite::Category::Callable, function->get_state());
  }
  case Code::Type::Enum: {
    auto enumeration =
        Interpreter::Types::Enumeration::parse(cursor, definition);
    BAIL_IF(!enumeration);
    return Result(
        enumeration->get_semantic(), Language::Types::Composite::Category::Type,
        enumeration->get_state());
  }
  case Code::Type::Namespace: {
    auto selected = Interpreter::Types::Namespace::parse(cursor, definition);
    BAIL_IF(!selected);
    return Result(
        selected->get_semantic(), Language::Types::Composite::Category::Type,
        selected->get_state());
  }
  case Code::Type::Struct: {
    auto structure = Interpreter::Types::Structure::parse(cursor, definition);
    BAIL_IF(!structure);
    return Result(
        structure->get_semantic(), Language::Types::Composite::Category::Type,
        structure->get_state());
  }
  case Code::Type::Object: {
    auto object = Interpreter::Types::Object::parse(cursor, definition);
    BAIL_IF(!object);
    return Result(
        object->get_semantic(), Language::Types::Composite::Category::Type,
        object->get_state());
  }
  case Code::Type::Interface: {
    auto interface = Interpreter::Types::Interface::parse(cursor, definition);
    BAIL_IF(!interface);
    return Result(
        interface->get_semantic(), Language::Types::Composite::Category::Type,
        interface->get_state());
  }
  case Code::Type::Implementation: {
    auto implemented =
        Interpreter::Types::Implemented::parse(cursor, definition);
    BAIL_IF(!implemented);
    return Result(
        implemented->get_semantic(), Language::Types::Composite::Category::Type,
        implemented->get_state());
  }
  default:
    cursor.create_token_error(
        qualifier,
        "Library members require a Type, `alias`, `namespace`, `enum`, "
        "`struct`, `object`, `interface`, `implementation`, "
        "`func`, or inferred initializer qualifier."_view);
    return {};
  }
}
