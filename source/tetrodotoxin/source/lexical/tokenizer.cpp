// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/lexical/tokenizer.hpp"

#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/source/lexical/lexicon.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source::Lexical;

// Context is the tokenizer cursor for one source view and one token stream.
// It owns the mutable scan coordinates because token helpers walk different
// shapes before they know the final Code, but all of them publish a compact
// Token alongside its full source range.
//
// Grammar helpers consume different spellings, while add_token records their
// common byte range and allocates the locator. Keeping that arithmetic here
// lets the helpers classify input without choosing a public token layout.
class Context {
 public:
  Context(const View::Bytes source, Allocator::Arena& arena)
      : source(source), tokens(arena) {
    // Take an estimated best guess on the token count. This helps save on
    // resizes even if we end up oversized.
    tokens.reset(source.get_size() / 4);
  };

  constexpr auto can_parse() const -> Bool {
    return parse_index < source.get_size();
  }

  // The main parse loop guards current token dispatch with can_parse.
  // Reading through the raw pointer keeps that hot path from repeating the
  // same bounds check for every switch and helper.
  constexpr auto current() const -> U8 {
    return source.get_data()[parse_index];
  }

  constexpr auto get_source() const -> View::Bytes { return source; }
  constexpr auto get_tokens() const -> View::Vector<Stream::Entry> {
    return tokens.get_view();
  }

  constexpr auto slice(Count start, Count size) const -> View::Bytes {
    return source.slice(start, size);
  }

  // Keyword and directive tables need the token bytes before the final Code
  // is known. Keep that as one current token view instead of exposing token
  // start and token size as separate pieces of state.
  constexpr auto current_token_text() const -> View::Bytes {
    return source.slice(token_start, parse_index - token_start);
  }

  // Lookahead often asks about a byte that might be past the end of the
  // source. View::Bytes owns that protected access, so Context does not need
  // to duplicate the same end of source logic.
  constexpr auto peek_ahead(U32 amount) const -> U8 {
    return source[parse_index + amount];
  }

  // Token helpers advance parse_index before they know the final Code.
  // Capturing the start here lets add_token build the final source
  // slice without leaking token start state to every helper.
  constexpr auto begin_token() -> void { token_start = parse_index; }

  // Fixed markers belong to the spelling unless a grammar helper explicitly
  // begins its token after the marker, as with an attribute name.
  constexpr auto strip_source_prefix(Code::Type type) -> void {
    advance_parse(Lexicon::get_spelling(type).get_size());
  }

  // Closes a range token with a possible closing symbol if available.
  template <Bool use_escape, Bool consume_terminal>
  constexpr auto parse_range(Code::Type type, U8 terminal_symbol) -> void {
    // Strip the fixed source prefix.
    strip_source_prefix(type);

    // Parse until we run out of valid text or we hit the closing symbol.
    while (can_parse() && current() != terminal_symbol) {
      // If escape characters are toggled then backslashes automatically consume
      // two characters which allows escaping terminal symbols.
      if constexpr (use_escape) {
        advance_parse(
            current() == '\\' && parse_index + 1 < source.get_size() ? 2 : 1);
      } else {
        advance_parse();
      }
    }

    if constexpr (consume_terminal) {
      if (can_parse() && current() == terminal_symbol) {
        advance_parse();
      }
    }

    add_token(type);
  }

  constexpr auto advance_parse(Count amount = 1) -> void {
    parse_index += amount;
  }

  // TODO: Only used for the range case which we haven't optimized quite yet.
  constexpr auto backup_parse() -> void { parse_index--; }

  constexpr auto consume_line_break() -> void { parse_index++; }

  constexpr auto add_token(Code::Type type) -> void {
    tokens.insert(
        Stream::Entry(
            Token(tokens.get_size(), type),
            Tetrodotoxin::Source::Range(
                token_start, parse_index - token_start)));
  }

  constexpr auto add_terminal() -> void {
    // Terminal occupies no source bytes and begins at the authored boundary.
    // Parsers can therefore use it as an exact end position without repairing
    // an offset beyond the source.
    token_start = parse_index;
    add_token(Code::Type::Terminal);
  }

 private:
  U64 token_start = 0;
  U64 parse_index = 0;
  const View::Bytes source;
  Managed::Vector<Stream::Entry> tokens;
};

static auto parse_attribute(Context& ctx) -> void {
  // Skip the '@' in the token name. Empty attributes are allowed in
  // tokenization but should be rejected by parsers.
  ctx.strip_source_prefix(Code::Type::Attribute);
  ctx.begin_token();
  while (Lexicon::is_identifier(ctx.peek_ahead(1))) {
    ctx.advance_parse();
  }

  ctx.advance_parse();
  ctx.add_token(Code::Type::Attribute);
}

static auto parse_number(Context& ctx) -> void {
  // If we start with zero then check if we have a valid hex sequence.
  if (ctx.current() == '0' && ctx.peek_ahead(1) == 'x') {
    switch (ctx.peek_ahead(2)) {
    // 0x[FF FF ...] hex byte array literal (any whitespace is fine)
    case '[': {
      ctx.parse_range<false, true>(Code::Type::Bytes, ']');
      return;
    }

      // 0xFF... hex integer literal.
    case '0' ... '9':
    case 'a' ... 'f':
    case 'A' ... 'F': {
      ctx.strip_source_prefix(Code::Type::Hex);
      while (ctx.can_parse() && Lexicon::is_hex(ctx.current())) {
        ctx.advance_parse();
      }

      ctx.add_token(Code::Type::Hex);
      return;
    }

      // Not a valid hex sequence
    default:
      break;
    }
  }

  Bool found_decimal = false;
  Code::Type type = Code::Type::Numeric;

  char numeric_char = ctx.peek_ahead(1);
  while (Lexicon::is_numeric(numeric_char)) {
    ctx.advance_parse();
    if (numeric_char == '.') {
      // Don't consume a '.' that starts a RangeOp '...'.
      if (ctx.peek_ahead(1) == '.') {
        ctx.backup_parse();
        break;
      }

      if (found_decimal) {
        ctx.backup_parse();
        break;
      }

      found_decimal = true;
      type = Code::Type::Float;
    }

    numeric_char = ctx.peek_ahead(1);
  }

  ctx.advance_parse();
  ctx.add_token(type);
}

static auto parse_type(Context& ctx) -> void {
  while (Lexicon::is_type(ctx.peek_ahead(1))) {
    ctx.advance_parse();
  }

  ctx.advance_parse();
  ctx.add_token(Code::Type::Type);
}

static auto parse_unknown(Context& ctx) -> void {
  ctx.advance_parse();
  ctx.add_token(Code::Type::Unknown);
}

static auto parse_identifier(Context& ctx) -> void {
  if (!Lexicon::is_identifier(ctx.peek_ahead(0))) {
    parse_unknown(ctx);
    return;
  }

  while (Lexicon::is_identifier(ctx.peek_ahead(1))) {
    ctx.advance_parse();
  }

  ctx.advance_parse();
  ctx.add_token(
      Lexicon::get_keyword(ctx.current_token_text(), Code::Type::Addressable));
}

template <Code::Type type>
static auto parse_simple(Context& ctx) -> void {
  constexpr Count token_length = Lexicon::get_spelling(type).get_size();
  ctx.advance_parse(token_length);
  ctx.add_token(type);
}

auto Tokenizer::parse(Allocator::Arena& arena) -> void {
  Context ctx(get_source_text(), arena);
  while (ctx.can_parse()) {
    ctx.begin_token();
    switch (ctx.current()) {
    case '\n':
      ctx.consume_line_break();
      break;

    case ' ':
    case '\t':
    case '\r':
      ctx.advance_parse();
      break;

    case '/':
      if (ctx.peek_ahead(1) == '/') {
        Code::Type type = ctx.peek_ahead(2) == '/' ? Code::Type::RawComment
                                                   : Code::Type::Comment;
        ctx.parse_range<false, false>(type, '\n');
        break;
      } else {
        parse_simple<Code::Type::DivOp>(ctx);
        break;
      }

    case '-':
      if (ctx.peek_ahead(1) == '>') {
        parse_simple<Code::Type::CallOp>(ctx);
        break;
      } else if (ctx.peek_ahead(1) == '=') {
        parse_simple<Code::Type::SubAssign>(ctx);
        break;
      } else {
        parse_simple<Code::Type::SubOp>(ctx);
        break;
      }

    case '+':
      if (ctx.peek_ahead(1) == '=') {
        parse_simple<Code::Type::AddAssign>(ctx);
        break;
      } else {
        parse_simple<Code::Type::AddOp>(ctx);
        break;
      }

    case '=':
      if (ctx.peek_ahead(1) == '=') {
        parse_simple<Code::Type::CmpOp>(ctx);
        break;
      } else {
        parse_simple<Code::Type::Assign>(ctx);
        break;
      }

    case '<':
      if (ctx.peek_ahead(1) == '=') {
        parse_simple<Code::Type::LessEqOp>(ctx);
        break;
      } else {
        parse_simple<Code::Type::LessOp>(ctx);
        break;
      }

    case '>':
      if (ctx.peek_ahead(1) == '=') {
        parse_simple<Code::Type::GreaterEqOp>(ctx);
        break;
      } else {
        parse_simple<Code::Type::GreaterOp>(ctx);
        break;
      }

    case '@':
      parse_attribute(ctx);
      break;

    // Tested faster to unroll number parsing here in the switch rather than
    // try and push it to the default.
    case '0' ... '9':
      parse_number(ctx);
      break;

    // All Addressable names are snake_case.
    case 'a' ... 'z':
      parse_identifier(ctx);
      break;

    // Type names are PascalCase. Attribute directives start with `@` and
    // are handled by parse_attribute instead.
    case 'A' ... 'Z':
      parse_type(ctx);
      break;

    case '"':
      ctx.parse_range<true, true>(Code::Type::String, '"');
      break;

    case '$':
      if (ctx.peek_ahead(1) == '[') {
        ctx.parse_range<false, true>(Code::Type::Embedded, ']');
      } else {
        parse_unknown(ctx);
      }

      break;

    case '[':
      parse_simple<Code::Type::BracketStart>(ctx);
      break;

    case ']':
      parse_simple<Code::Type::BracketEnd>(ctx);
      break;

    case ')':
      parse_simple<Code::Type::PackingEnd>(ctx);
      break;

    case '.':
      if (ctx.peek_ahead(1) == '[') {
        parse_simple<Code::Type::SwizzleOp>(ctx);
        break;
      } else if (ctx.peek_ahead(1) == '.' && ctx.peek_ahead(2) == '.') {
        parse_simple<Code::Type::RangeOp>(ctx);
        break;
      } else {
        parse_simple<Code::Type::AddressOp>(ctx);
        break;
      }

    case '!':
      if (ctx.peek_ahead(1) == '=') {
        parse_simple<Code::Type::NotEqOp>(ctx);
      } else {
        parse_simple<Code::Type::NotOp>(ctx);
      }

      break;

    case '?':
      parse_simple<Code::Type::QuestionOp>(ctx);
      break;

    case ':':
      if (ctx.peek_ahead(1) == '[') {
        parse_simple<Code::Type::ValueAccessOp>(ctx);
      } else if (ctx.peek_ahead(1) == ':') {
        parse_simple<Code::Type::TypeAccessOp>(ctx);
      } else {
        parse_simple<Code::Type::Define>(ctx);
      }

      break;

      // Simple spot tokens
    case '{':
      parse_simple<Code::Type::ScopeStart>(ctx);
      break;
    case '}':
      parse_simple<Code::Type::ScopeEnd>(ctx);
      break;
    case '(':
      parse_simple<Code::Type::PackingStart>(ctx);
      break;
    case '*':
      parse_simple<Code::Type::MulOp>(ctx);
      break;
    case '%':
      parse_simple<Code::Type::ModOp>(ctx);
      break;
    case '&':
      parse_simple<Code::Type::AndOp>(ctx);
      break;
    case '|':
      parse_simple<Code::Type::OrOp>(ctx);
      break;
    case ';':
      parse_simple<Code::Type::EndStatement>(ctx);
      break;
    case '_':
      parse_simple<Code::Type::Discard>(ctx);
      break;
    case ',':
      parse_simple<Code::Type::PackingOp>(ctx);
      break;

      // We failed to parse so log the unknown token as we don't want to drop it
      // from the stream. Sometimes a format or another command is issued to the
      // LSP speculatively and it's super annoying if the formatter drops
      // partial tokens that were a work in progress.
    default:
      parse_unknown(ctx);
      break;
    }
  }

  ctx.add_terminal();
  set_entries(ctx.get_tokens());
}
