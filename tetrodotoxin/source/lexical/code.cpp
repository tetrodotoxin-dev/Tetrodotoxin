// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/lexical/code.hpp"

#include "perimortem/core/null_terminated.hpp"

auto Tetrodotoxin::Source::Lexical::Code::get_semantics() const
    -> Perimortem::Core::View::Bytes {
  switch (type) {
  // Modifiers
  case Type::Public:
    return "public publication modifier"_view;
  case Type::Private:
    return "private publication modifier"_view;
  case Type::Expose:
    return "expose publication modifier"_view;
  case Type::State:
    return "state evaluation modifier"_view;
  case Type::Const:
    return "const evaluation modifier"_view;

  // Definition keywords
  case Type::Alias:
    return "Alias definition"_view;
  case Type::Namespace:
    return "Namespace definition"_view;
  case Type::Enum:
    return "Enumeration definition"_view;
  case Type::Struct:
    return "Structure definition"_view;
  case Type::Object:
    return "Object definition"_view;
  case Type::Interface:
    return "Interface definition"_view;
  case Type::Implementation:
    return "Interface implementation definition"_view;

  // Statement and import keywords
  case Type::Using:
    return "context forwarding"_view;
  case Type::New:
    return "Object initializer"_view;
  case Type::If:
    return "conditional branch"_view;
  case Type::In:
    return "iteration relation"_view;
  case Type::For:
    return "iteration loop"_view;
  case Type::Break:
    return "loop break"_view;
  case Type::Continue:
    return "loop continuation"_view;
  case Type::Match:
    return "pattern match"_view;
  case Type::Case:
    return "match case"_view;
  case Type::Else:
    return "alternate branch"_view;
  case Type::While:
    return "conditional loop"_view;
  case Type::Return:
    return "return statement"_view;
  case Type::Emit:
    return "emission keyword"_view;
  case Type::Source:
    return "source import"_view;
  case Type::Package:
    return "Package import"_view;
  case Type::Dialect:
    return "source Dialect selection"_view;
  case Type::Func:
    return "Callable definition"_view;
  // Binary operators
  case Type::AddOp:
    return "addition operator"_view;
  case Type::SubOp:
    return "subtraction operator"_view;
  case Type::MulOp:
    return "multiplication operator"_view;
  case Type::DivOp:
    return "division operator"_view;
  case Type::ModOp:
    return "remainder operator"_view;
  case Type::CmpOp:
    return "equality comparison operator"_view;
  case Type::NotEqOp:
    return "inequality comparison operator"_view;
  case Type::LessOp:
    return "less than comparison operator"_view;
  case Type::GreaterOp:
    return "greater than comparison operator"_view;
  case Type::LessEqOp:
    return "less than or equal comparison operator"_view;
  case Type::GreaterEqOp:
    return "greater than or equal comparison operator"_view;
  case Type::And:
    return "logical conjunction operator"_view;
  case Type::Or:
    return "logical disjunction operator"_view;
  case Type::AndOp:
    return "reserved bitwise conjunction operator"_view;
  case Type::OrOp:
    return "reserved bitwise disjunction operator"_view;

  // Assignment operators
  case Type::Assign:
    return "assignment operator"_view;
  case Type::AddAssign:
    return "addition assignment operator"_view;
  case Type::SubAssign:
    return "subtraction assignment operator"_view;

  // Address, packing, and parser operators
  case Type::ScopeStart:
    return "scope start"_view;
  case Type::ScopeEnd:
    return "scope end"_view;
  case Type::PackingStart:
    return "packing start"_view;
  case Type::PackingEnd:
    return "packing end"_view;
  case Type::BracketStart:
    return "bracket start"_view;
  case Type::BracketEnd:
    return "bracket end"_view;
  case Type::Define:
    return "definition separator"_view;
  case Type::TypeAccessOp:
    return "Type context access operator"_view;
  case Type::EndStatement:
    return "statement terminator"_view;
  case Type::CallOp:
    return "Callable invocation operator"_view;
  case Type::RangeOp:
    return "range operator"_view;
  case Type::PackingOp:
    return "packing separator"_view;
  case Type::AddressOp:
    return "Addressable access operator"_view;
  case Type::SwizzleOp:
    return "swizzle access operator"_view;
  case Type::ValueAccessOp:
    return "safe value access operator"_view;
  case Type::NotOp:
    return "logical negation operator"_view;
  case Type::QuestionOp:
    return "propagation operator"_view;
  case Type::Discard:
    return "discard value"_view;

  // Other fixed keywords and markers
  case Type::Self:
    return "self reference"_view;
  case Type::True:
    return "Flag literal true"_view;
  case Type::False:
    return "Flag literal false"_view;

  // Literal markers
  case Type::Bytes:
    return "Bytes literal"_view;
  case Type::Embedded:
    return "embedded source literal"_view;

  // Other fixed markers
  case Type::Attribute:
    return "Attribute name"_view;
  case Type::Comment:
    return "documentation comment"_view;
  case Type::RawComment:
    return "raw source comment"_view;

  // Source carried groupings
  case Type::String:
    return "quoted Bytes literal"_view;
  case Type::Numeric:
    return "U64 literal"_view;
  case Type::Hex:
    return "U64 hexadecimal literal"_view;
  case Type::Float:
    return "R64 literal"_view;
  case Type::Addressable:
    return "Addressable space name"_view;
  case Type::Type:
    return "Type space name"_view;
  case Type::PackedData:
    return "packed data source"_view;
  case Type::Terminal:
    return "terminal Code"_view;

  // Unknown is the prescribed fallback for any source without a richer
  // semantic grouping in this Lexer contract.
  case Type::Unknown:
    return "unknown source Code"_view;
  }

  return "unknown source Code"_view;
}
