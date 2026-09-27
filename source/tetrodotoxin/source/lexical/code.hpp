// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

namespace Tetrodotoxin::Source::Lexical {

// Code is one byte emitted by a concrete Lexer. It is the semantic handshake
// between that Lexer and consumers of its token stream. Every emitted token has
// one Code and every Code identifies one prescribed grouping in the concrete
// Lexer contract.
//
// Terminal and Unknown reserve 0x00 and 0xFF. Every other value belongs to the
// concrete Lexer contract and may be remapped whenever that contract changes.
class Code {
 public:
  enum class Type : U8 {
    // ========================================================================
    //                             Control Types
    //
    //       These are the only two reserved values in every Code space.
    // ========================================================================
    // 0xFF represents source without a richer semantic grouping.
    Unknown = 0xFF,
    // 0x00 terminates the token stream.
    Terminal = 0x00,

    // ========================================================================
    //                              TTX Data Model
    // ========================================================================
    Comment,       // //
    RawComment,    // ///
    Attribute,     // @
    Addressable,   // Any symbol that starts with a lowercase ASCII letter
    Type,          // Any symbol that starts with an uppercase ASCII letter
    EndStatement,  // Statement terminator

    // ========================================================================
    //                              Data objects
    // ========================================================================
    Numeric,   // Decimal digits without a point
    Hex,       // Hexadecimal digits after 0x
    Float,     // Decimal digits with one point
    String,    // Quoted Bytes without an implicit null terminator
    Bytes,     // 0x[FF FF FF]
    Embedded,  // $[path/to/file]
    Discard,   // Discard (_)
    PackedData,

    // ========================================================================
    //                              TTX Pairs
    // ========================================================================
    ScopeStart,    // {
    ScopeEnd,      // }
    PackingStart,  // (
    PackingEnd,    // )
    BracketStart,  // [
    BracketEnd,    // ]

    // ========================================================================
    //                               Operators
    // ========================================================================
    AddOp,          // +
    SubOp,          // Subtraction
    DivOp,          // /
    MulOp,          // *
    ModOp,          // %
    LessOp,         // <
    GreaterOp,      // >
    LessEqOp,       // <=
    GreaterEqOp,    // >=
    CmpOp,          // ==
    NotEqOp,        // !=
    Assign,         // =
    AddAssign,      // +=
    SubAssign,      // Subtraction assignment
    CallOp,         // Callable access
    AddressOp,      // .
    SwizzleOp,      // .[
    ValueAccessOp,  // :[
    PackingOp,      // ,
    NotOp,          // !
    QuestionOp,     // ?
    RangeOp,        // ...
    Define,         // :
    TypeAccessOp,   // ::
    // These Codes reserve the bitwise operator groupings without prescribing
    // how a consumer evaluates them.
    AndOp,  // & reserved
    OrOp,   // | reserved

    // ========================================================================
    //                            Fixed Keywords
    // ========================================================================
    // Keywords are names in the Addressable source space that the Lexer
    // promotes into distinct semantic Codes. This reserves those spellings from
    // generic Addressable dispatch and lets consumers use the prescribed
    // grouping without inspecting source text again.
    And,
    Or,
    If,
    In,
    For,
    While,
    Case,
    Match,
    Break,
    Continue,
    Else,
    Func,
    Self,
    True,
    False,
    Return,
    Emit,
    Source,
    Package,
    Dialect,
    Alias,
    Namespace,
    Enum,
    Struct,
    Object,
    Interface,
    Implementation,
    Using,
    New,

    // Modifiers receive distinct Codes because publication and evaluation are
    // prescribed groupings in this Lexer contract. Consumers can accept or
    // reject those groupings without rediscovering them from Addressable text.
    Public,
    Private,
    Expose,
    State,
    Const,
  };

  constexpr Code() = default;
  constexpr Code(Type type) : type(type) {}

  constexpr auto operator==(const Code& rhs) const -> Bool {
    return get_type() == rhs.get_type();
  }

  constexpr auto operator==(Type type) const -> Bool {
    return get_type() == type;
  }

  constexpr auto operator!=(const Code& rhs) const -> Bool {
    return get_type() != rhs.get_type();
  }

  constexpr auto operator!=(Type type) const -> Bool {
    return get_type() != type;
  }

  constexpr auto is_one_of(Perimortem::Core::View::Vector<Type> values) const
      -> Bool {
    return values.contains(get_type());
  }

  constexpr auto is_publication_modifier() const -> Bool {
    return type == Type::Public || type == Type::Private ||
           type == Type::Expose;
  }

  constexpr auto is_evaluation_modifier() const -> Bool {
    return type == Type::State || type == Type::Const;
  }

  constexpr auto is_modifier() const -> Bool {
    return is_publication_modifier() || is_evaluation_modifier();
  }

  constexpr auto is_comment() const -> Bool {
    return type == Type::Comment || type == Type::RawComment;
  }

  constexpr auto get_type() const -> Type { return type; }

  // Describes the prescribed semantic grouping encoded by this Code.
  auto get_semantics() const -> Perimortem::Core::View::Bytes;

 private:
  Type type = Type::Terminal;
};

static_assert(static_cast<U8>(Code::Type::Terminal) == 0x00);
static_assert(static_cast<U8>(Code::Type::Unknown) == 0xFF);

}  // namespace Tetrodotoxin::Source::Lexical
