// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/literal.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/reader/textual.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/path.hpp"

#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/generics/fixed.hpp"
#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/source/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;

static auto materialize_bytes_type(
    const Abstract& context,
    Cursor& cursor,
    Span span,
    Count size) -> Option<const Library::Language::Model::Type&> {
  if (size == 0) {
    auto byte = context.resolve_concept("U8"_view)
                    .select<Library::Language::Model::Type>();
    auto generic = context.resolve_concept("View"_view)
                       .select<Library::Language::Generic>();
    BAIL_IF(!byte || !generic);
    Static::Vector<Library::Language::Generic::Argument, 1> arguments = {{
      Library::Language::Generic::Argument(*byte),
    }};
    return generic->materialize(arguments.get_view())
        .visit(
            [](const Library::Language::Model::Type& type)
                -> Option<const Library::Language::Model::Type&> {
              return type;
            },
            [&](const Library::Language::Generic::Failure&)
                -> Option<const Library::Language::Model::Type&> {
              cursor.create_expression_error(
                  span,
                  "Library could not materialize `View[U8]` for this "
                  "empty Bytes literal."_view,
                  "Check that View and canonical U8 are available."_view);
              return {};
            });
  }

  // Keep the max array length the same as what the Bibliotheca can manage.
  constexpr U64 max_extent = U64(1) << 36;
  if (size > Count(max_extent)) {
    cursor.create_expression_error(
        span, "Bytes literal exceeds Library's Fixed extent range."_view,
        "Reduce the byte count below the 68,719,476,736 extent limit."_view);
    return {};
  }

  auto byte = context.resolve_concept("U8"_view)
                  .select<Library::Language::Model::Type>();
  auto generic = context.resolve_concept("Fixed"_view)
                     .select<Library::Language::Generic>();
  BAIL_IF(!byte || !generic);
  Static::Vector<Library::Language::Generic::Argument, 2> arguments = {{
    Library::Language::Generic::Argument(*byte),
    Library::Language::Generic::Argument(U64(size)),
  }};
  return generic->materialize(arguments.get_view())
      .visit(
          [](const Library::Language::Model::Type& type)
              -> Option<const Library::Language::Model::Type&> { return type; },
          [&](const Library::Language::Generic::Failure&)
              -> Option<const Library::Language::Model::Type&> {
            auto report = cursor.create_report(span);
            report << "Library could not materialize `Fixed[U8, "_view
                   << U64(size) << "]` for this Bytes literal."_view;
            report.get_hint()
                << "Check that Fixed and canonical U8 are available."_view;
            return {};
          });
}

static auto construct_retained_bytes(
    Allocator::Arena& domain,
    const Abstract& context,
    Cursor& cursor,
    Span span,
    View::Bytes value,
    Option<const Tetrodotoxin::Language::Resource&> resource = {})
    -> Option<Library::Language::Constant&> {
  auto type = materialize_bytes_type(context, cursor, span, value.get_size());
  return type.visit(
      []() -> Option<Library::Language::Constant&> { return {}; },
      [&](const Library::Language::Model::Type& type)
          -> Option<Library::Language::Constant&> {
        Anchor anchor = Anchor::create(span.get_start(), span);

        cursor.consume();
        return Library::Language::Constants::Bytes::create_authored(
            domain, type, value, anchor, resource);
      });
}

static auto parse_quoted(
    Allocator::Arena& domain,
    const Abstract& context,
    Cursor& cursor) -> Option<Library::Language::Constant&> {
  Span literal_span(cursor.current());
  View::Bytes text = literal_span.caculate_text(cursor.get_source_text());
  View::Bytes payload = text.slice(1, text.get_size() - 2);
  Count decoded_size = payload.get_size();

  // Escape markers belong to the authored spelling rather than the retained
  // value. Count them before allocation so the Arena receives the exact fact.
  for (Count i = 0; i < payload.get_size(); i++) {
    if (payload[i] == '\\') {
      decoded_size--;
      i++;
    }
  }

  auto decoded = cursor.get_arena().allocate(decoded_size);
  auto* decoded_data = decoded.get_data();
  Count output = 0;
  for (Count i = 0; i < payload.get_size(); i++) {
    if (payload[i] == '\\') {
      i++;
    }

    decoded_data[output] = payload[i];
    output++;
  }

  return construct_retained_bytes(
      domain, context, cursor, literal_span,
      View::Bytes(decoded.get_data(), decoded.get_size()));
}

static auto parse_byte_array(
    Allocator::Arena& domain,
    const Abstract& context,
    Cursor& cursor) -> Option<Library::Language::Constant&> {
  Span literal_span(cursor.current());
  View::Bytes text = literal_span.caculate_text(cursor.get_source_text());
  View::Bytes payload = text.slice(3, text.get_size() - 4);
  Count digits = 0;

  // Whitespace separates authored digits but contributes no retained byte.
  // Count first so malformed pairs never publish a partial Constant.
  for (Count i = 0; i < payload.get_size(); i++) {
    U8 value = payload[i];
    Bool hexadecimal = Lexicon::is_hex(value);
    if (!hexadecimal && !Lexicon::is_whitespace(value)) {
      cursor.create_expression_error(
          literal_span, "Bytes literal contains a non hexadecimal digit."_view,
          "Use hexadecimal pairs containing only 0 through 9 and A through "
          "F."_view);
      return {};
    }

    digits += hexadecimal ? 1 : 0;
  }

  if (digits % 2 != 0) {
    cursor.create_expression_error(
        literal_span,
        "Bytes literal ends with an incomplete hexadecimal byte."_view,
        "Add or remove one hexadecimal digit so every byte has two digits."_view);
    return {};
  }

  auto decoded = cursor.get_arena().allocate(digits / 2);
  auto* decoded_data = decoded.get_data();
  Count nibble = 0;
  U8 byte = 0;
  for (Count i = 0; i < payload.get_size(); i++) {
    if (Lexicon::is_whitespace(payload[i])) {
      continue;
    }

    if (nibble % 2 == 0) {
      byte = U8(Lexicon::get_hex_value(payload[i]) << 4);
    } else {
      decoded_data[nibble / 2] = byte | Lexicon::get_hex_value(payload[i]);
    }

    nibble++;
  }

  return construct_retained_bytes(
      domain, context, cursor, literal_span,
      View::Bytes(decoded.get_data(), decoded.get_size()));
}

static auto parse_embedded(
    Allocator::Arena& domain,
    const Abstract& context,
    Cursor& cursor,
    const Abstract& source_context) -> Option<Library::Language::Constant&> {
  Span literal_span(cursor.current());
  View::Bytes route = literal_span.caculate_text(cursor.get_source_text());

  View::Bytes authored = route.slice(2, route.get_size() - 3);
  Perimortem::System::Path canonical(cursor.get_logical_path(), authored);
  if (canonical.get_view().is_empty() || canonical.is_rooted()) {
    cursor.create_expression_error(
        literal_span,
        "Embedded literal did not resolve to one confined relative path."_view,
        "Use a path relative to this source without escaping the Package root."_view);
    return {};
  }
  Managed::Bytes canonical_route(domain, "$["_view);
  canonical_route.concat(canonical.get_view());
  canonical_route.append(']');

  const Abstract& selected =
      source_context.resolve_concept(canonical_route.get_view()).resolve();
  auto error = selected.select<Tetrodotoxin::Language::Error>();
  if (error) {
    auto report = cursor.create_report(literal_span);
    error->describe(report);
    return {};
  }

  auto resource = selected.select<Tetrodotoxin::Language::Resource>();
  if (!resource) {
    cursor.create_expression_error(
        literal_span,
        "Embedded literal did not resolve to a Package Resource."_view,
        "Check the source relative route and confirm the Resource exists."_view);
    return {};
  }

  View::Bytes retained = resource->get_value();

  // Resource keeps its backing stable for the caller domain. Borrow it directly
  // so same domain imports retain one allocation for the semantic island.
  return construct_retained_bytes(
      domain, context, cursor, literal_span, retained, *resource);
}

// Tokenization has already selected the Flag domain. Literal therefore uses
// the Code directly and introduces no second truth spelling policy.
static auto parse_flag(
    Allocator::Arena& domain,
    const Abstract& context,
    Cursor& cursor) -> Option<Library::Language::Constant&> {
  Token token = cursor.consume();
  Anchor anchor = Anchor::create(token, Span(token));

  auto type =
      context.resolve_concept("Bool"_view)
          .select<Tetrodotoxin::Library::Language::Model::Types::Flag>();
  BAIL_IF(!type);
  if (token.get_code() == Code::Type::True) {
    return Library::Language::Constants::True::create_authored(
        domain, *type, anchor);
  }

  return Library::Language::Constants::False::create_authored(
      domain, *type, anchor);
}

template <Count radix>
static auto parse_unsigned(
    Allocator::Arena& domain,
    const Abstract& context,
    Cursor& cursor) -> Option<Library::Language::Constant&> {
  Span literal_text(cursor.current());
  Reader::Textual reader(literal_text.caculate_text(cursor.get_source_text())
                             .slice(radix == 16 ? 2 : 0));

  // Textual must consume the complete Token payload. Accepting a valid prefix
  // would publish a different value for malformed authored bytes.
  U64 value = reader.read_unsigned(radix);
  if (!reader.is_valid() || reader.get_location() != reader.get_size()) {
    cursor.create_expression_error(
        literal_text, "Unable to parse unsigned literal value."_view);
    return {};
  }

  // Consumption follows complete validation so failure leaves the Cursor at
  // the literal that needs the diagnostic.
  Anchor anchor = Anchor::create(literal_text.get_start(), literal_text);

  cursor.consume();
  auto type =
      context.resolve_concept("U64"_view)
          .select<Tetrodotoxin::Library::Language::Model::Types::Unsigned>();
  BAIL_IF(!type);
  return Library::Language::Constants::Unsigned::create_authored(
      domain, *type, value, anchor);
}

static auto parse_signed(
    Allocator::Arena& domain,
    const Abstract& context,
    Cursor& cursor) -> Option<Library::Language::Constant&> {
  Span literal_text(cursor.current(), cursor.peek(1));
  Reader::Textual reader(literal_text.caculate_text(cursor.get_source_text()));

  // The leading sign and digits form one semantic value even though the Lexer
  // exposes two Tokens. Textual must reject any unconsumed authored bytes.
  S64 value = reader.read_signed();
  if (!reader.is_valid() || reader.get_location() != reader.get_size()) {
    cursor.create_expression_error(
        literal_text, "Unable to parse signed literal value."_view);
    return {};
  }

  // Both Tokens advance only after the complete value parses.
  Anchor anchor = Anchor::create(literal_text.get_start(), literal_text);

  cursor.consume();
  cursor.consume();
  auto type =
      context.resolve_concept("S64"_view)
          .select<Tetrodotoxin::Library::Language::Model::Types::Signed>();
  BAIL_IF(!type);
  return Library::Language::Constants::Signed::create_authored(
      domain, *type, value, anchor);
}

template <S64 token_width>
static auto parse_real(
    Allocator::Arena& domain,
    const Abstract& context,
    Cursor& cursor) -> Option<Library::Language::Constant&> {
  Span literal_text(cursor.current(), cursor.peek(token_width - 1));
  Reader::Textual reader(literal_text.caculate_text(cursor.get_source_text()));

  // Textual sees the complete signed or unsigned spelling so partial numeric
  // acceptance cannot change the Constant represented by the source.
  R64 value = reader.read_r64();
  if (!reader.is_valid() || reader.get_location() != reader.get_size()) {
    cursor.create_expression_error(
        literal_text, "Unable to parse real literal value."_view);
    return {};
  }

  // A negative Real owns its sign Token too. Consume the exact lexical width
  // only after validation proves one complete authored Constant.
  Anchor anchor = Anchor::create(literal_text.get_start(), literal_text);

  for (S64 i = 0; i < token_width; i++) {
    cursor.consume();
  }

  auto type =
      context.resolve_concept("R64"_view)
          .select<Tetrodotoxin::Library::Language::Model::Types::Real>();
  BAIL_IF(!type);
  return Library::Language::Constants::Real::create_authored(
      domain, *type, value, anchor);
}

auto Library::Interpreter::Literal::parse(
    const Abstract& context,
    Cursor& cursor) -> Option<Library::Language::Constant&> {
  Allocator::Arena& domain = cursor.get_arena();
  const Abstract& source_context = context;

  // A leading subtraction spelling admits only signed decimal and Real
  // literals. Without it the ordinary unsigned parser keeps its full domain.
  if (cursor.matches(Code::Type::SubOp)) {
    switch (cursor.peek(1).get_code().get_type()) {
    case Code::Type::Numeric:
      return parse_signed(domain, context, cursor);
    case Code::Type::Float:
      return parse_real<2>(domain, context, cursor);
    default:
      cursor.create_expression_error(
          Span(cursor.current(), cursor.peek(1)),
          "A negative literal requires a decimal integer or real."_view,
          "Use decimal spelling after `-` or remove the negative sign."_view);
      return {};
    }
  }

  // Literal selection observes the current Token without consuming a second
  // grammar path. Each concrete helper advances the same Cursor so a signed
  // value retains its complete Span and one diagnostic stream.
  switch (cursor.get_code().get_type()) {
  case Code::Type::String:
    return parse_quoted(domain, context, cursor);
  case Code::Type::Bytes:
    return parse_byte_array(domain, context, cursor);
  case Code::Type::True:
  case Code::Type::False:
    return parse_flag(domain, context, cursor);
  case Code::Type::Numeric:
    return parse_unsigned<10>(domain, context, cursor);
  case Code::Type::Hex:
    return parse_unsigned<16>(domain, context, cursor);
  case Code::Type::Float:
    return parse_real<1>(domain, context, cursor);
  case Code::Type::Embedded:
    return parse_embedded(domain, context, cursor, source_context);
  default:
    cursor.create_token_error(
        "Library literal parser requires a supported literal operand."_view,
        "Use a string, byte array, flag, integer, real, or embedded "
        "Resource literal."_view);
    return {};
  }
}
