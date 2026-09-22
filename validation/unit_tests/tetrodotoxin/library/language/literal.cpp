// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/literal.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Validation;

static Harness LiteralTests = {
  .name = "Tetrodotoxin::Library::Language::Literal"_view,
};

class LiteralResource : public Tetrodotoxin::Language::Resource {
 public:
  LiteralResource(Allocator::Arena& domain, View::Bytes value)
      : value(domain.proxy(value)) {}
  auto get_value() const -> View::Bytes override { return value; }

 private:
  View::Bytes value;
};

class LiteralError : public Tetrodotoxin::Language::Error {
 public:
  auto describe(Errors::Report& report) const -> void override {
    report << "Package supplied literal failure."_view;
  }
};

struct LiteralObservations {
  Bool table_seen = False;
  Bool empty_seen = False;
  Bool error_seen = False;
};

class LiteralContext : public Abstract {
 public:
  LiteralContext(Allocator::Arena& domain, LiteralObservations& observations)
      : observations(observations),
        table(domain, "0123456789"_view),
        empty(domain, View::Bytes()) {}

  auto get_name() const -> View::Bytes override { return "Context"_view; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes route) const -> const Abstract& override {
    if (route == "$[table]"_view) {
      observations.table_seen = True;
      return table;
    }

    if (route == "$[empty]"_view) {
      observations.empty_seen = True;
      return empty;
    }

    if (route == "$[error]"_view) {
      observations.error_seen = True;
      return error;
    }

    if (route == "$[other]"_view) {
      return *this;
    }

    return Unknown::get_unknown();
  }

  LiteralObservations& observations;
  LiteralResource table;
  LiteralResource empty;
  LiteralError error;
};

static auto create_monograph(
    Allocator::Arena& domain,
    Library::Dialect& dialect,
    Abstract& context) -> Option<Library::Language::Monograph&> {
  Errors errors;
  Tokenizer tokenizer(domain, ""_view, "literal-source.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  Anchor source_anchor = Anchor::create(Span());
  auto interpretation = dialect.interpret(
      cursor, Tetrodotoxin::Source::Documentation::get_empty(), source_anchor, context);
  BAIL_IF(!interpretation || !errors.is_empty());
  return interpretation->select<Library::Language::Monograph>();
}

static auto matches_token(const Cursor& cursor, Token expected) -> Bool {
  Token current = cursor.current();
  return current.get_offset() == expected.get_offset() &&
         current.get_code() == expected.get_code();
}

static auto parse_one(
    Allocator::Arena& domain,
    const Abstract& context,
    View::Bytes source,
    Errors& errors) -> Option<Library::Language::Constant&> {
  Tokenizer tokenizer(domain, source, "literal.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto parsed = Library::Interpreter::Literal::parse(context, cursor);
  if (parsed && !cursor.matches(Code::Type::Terminal)) {
    return {};
  }

  return parsed;
}

static auto rejects(
    Allocator::Arena& domain,
    const Abstract& context,
    View::Bytes source) -> Bool {
  Errors errors;
  Tokenizer tokenizer(domain, source, "rejected-literal.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  Token start = cursor.current();
  auto parsed = Library::Interpreter::Literal::parse(context, cursor);
  return !parsed && matches_token(cursor, start) && !errors.is_empty();
}

static auto render_rejection(
    Allocator::Arena& domain,
    const Abstract& context,
    View::Bytes source) -> Dynamic::Bytes {
  Errors errors;
  Tokenizer tokenizer(domain, source, "diagnostic-literal.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  Token start = cursor.current();
  auto parsed = Library::Interpreter::Literal::parse(context, cursor);
  if (parsed || !matches_token(cursor, start) || errors.get_size() != 1) {
    return Dynamic::Bytes("unexpected literal diagnostic state"_view);
  }

  Allocator::Arena render_arena;
  return Dynamic::Bytes(errors.render_message(render_arena, 0));
}

static auto contains(View::Bytes text, View::Bytes fragment) -> Bool {
  return Algorithm::search(text, fragment) != Count(-1);
}

PERIMORTEM_UNIT_TEST(LiteralTests, scalar_inference) {
  Allocator::Arena domain;
  LiteralObservations observations;
  LiteralContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  auto& graph = *monograph;
  Errors errors;
  Tokenizer tokenizer(
      domain, "true false 42 0x2A -7 1.5"_view, "scalars.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  auto true_value = Library::Interpreter::Literal::parse(graph, cursor);
  auto false_value = Library::Interpreter::Literal::parse(graph, cursor);
  auto decimal = Library::Interpreter::Literal::parse(graph, cursor);
  auto hexadecimal = Library::Interpreter::Literal::parse(graph, cursor);
  auto signed_value = Library::Interpreter::Literal::parse(graph, cursor);
  auto real = Library::Interpreter::Literal::parse(graph, cursor);

  ASSERT(
      true_value && false_value && decimal && hexadecimal && signed_value &&
      real);
  ASSERT(true_value->is<Library::Language::Constants::Flag>());
  EXPECT(true_value->is<Library::Language::Constants::True>());
  EXPECT_NOT(true_value->is<Library::Language::Constants::False>());
  ASSERT(false_value->is<Library::Language::Constants::Flag>());
  EXPECT(false_value->is<Library::Language::Constants::False>());
  EXPECT_NOT(false_value->is<Library::Language::Constants::True>());
  ASSERT(decimal->is<Library::Language::Constants::Unsigned>());
  ASSERT(hexadecimal->is<Library::Language::Constants::Unsigned>());
  ASSERT(signed_value->is<Library::Language::Constants::Signed>());
  ASSERT(real->is<Library::Language::Constants::Real>());
  EXPECT(
      static_cast<const Library::Language::Constants::Flag&>(*true_value)
          .get_value());
  EXPECT_NOT(
      static_cast<const Library::Language::Constants::Flag&>(*false_value)
          .get_value());
  EXPECT(&true_value->get_type() == &resolve_library_flag(graph));
  EXPECT(
      static_cast<const Library::Language::Constants::Unsigned&>(*decimal)
          .get_value() == 42);
  EXPECT(
      static_cast<const Library::Language::Constants::Unsigned&>(*hexadecimal)
          .get_value() == 42);
  EXPECT(&decimal->get_type() == &resolve_library_unsigned(graph, "U64"_view));
  EXPECT(
      &hexadecimal->get_type() == &resolve_library_unsigned(graph, "U64"_view));
  EXPECT(
      static_cast<const Library::Language::Constants::Signed&>(*signed_value)
          .get_value() == -7);
  EXPECT(
      &signed_value->get_type() == &resolve_library_signed(graph, "S64"_view));
  EXPECT(
      static_cast<const Library::Language::Constants::Real&>(*real)
          .get_value() == R64(1.5));
  EXPECT(&real->get_type() == &resolve_library_real(graph, "R64"_view));
  EXPECT(cursor.matches(Code::Type::Terminal));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LiteralTests, byte_domains) {
  Allocator::Arena domain;
  LiteralObservations observations;
  LiteralContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  auto& graph = *monograph;
  Errors errors;
  Dynamic::Bytes source("\"a\\\"b\" 0x[54\t54\n58\r31] \"\""_view);
  Tokenizer tokenizer(domain, source, "bytes.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  auto quoted = Library::Interpreter::Literal::parse(graph, cursor);
  auto hexadecimal = Library::Interpreter::Literal::parse(graph, cursor);
  auto empty = Library::Interpreter::Literal::parse(graph, cursor);

  ASSERT(quoted && hexadecimal && empty);
  ASSERT(quoted->is<Library::Language::Constants::Bytes>());
  ASSERT(hexadecimal->is<Library::Language::Constants::Bytes>());
  ASSERT(empty->is<Library::Language::Constants::Bytes>());
  source.set('x');
  const auto& quoted_bytes =
      static_cast<const Library::Language::Constants::Bytes&>(*quoted);
  const auto& hexadecimal_bytes =
      static_cast<const Library::Language::Constants::Bytes&>(*hexadecimal);
  const auto& empty_bytes =
      static_cast<const Library::Language::Constants::Bytes&>(*empty);
  EXPECT_TEXT(quoted_bytes.get_value(), "a\"b"_view);
  EXPECT_TEXT(hexadecimal_bytes.get_value(), "TTX1"_view);
  EXPECT(empty_bytes.get_value().is_empty());
  ASSERT(quoted->get_type().is<Library::Language::Types::Fixed>());
  const auto& quoted_type =
      static_cast<const Library::Language::Types::Fixed&>(quoted->get_type());
  EXPECT(quoted_type.get_extent() == 3);
  EXPECT(
      &quoted_type.get_element_type() ==
      &resolve_library_unsigned(graph, "U8"_view));
  EXPECT(cursor.matches(Code::Type::Terminal));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LiteralTests, r64_domain) {
  Allocator::Arena domain;
  LiteralObservations observations;
  LiteralContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  auto& source = *monograph;
  Errors tiny_errors;
  Errors wide_errors;

  auto tiny = parse_one(
      domain, source,
      "0.000000000000000000000000000000000000000000000000001"_view,
      tiny_errors);
  auto wide = parse_one(
      domain, source, "999999999999999999999999999999999999999.0"_view,
      wide_errors);

  ASSERT(tiny && wide);
  ASSERT(tiny->is<Library::Language::Constants::Real>());
  ASSERT(wide->is<Library::Language::Constants::Real>());
  EXPECT(&tiny->get_type() == &resolve_library_real(source, "R64"_view));
  EXPECT(&wide->get_type() == &resolve_library_real(source, "R64"_view));
  EXPECT(
      static_cast<const Library::Language::Constants::Real&>(*tiny)
          .get_value() > R64(0));
  EXPECT(
      static_cast<const Library::Language::Constants::Real&>(*wide)
          .get_value() > R64(1e38));
  EXPECT(tiny_errors.is_empty());
  EXPECT(wide_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LiteralTests, diagnostic_feedback) {
  Allocator::Arena domain;
  LiteralObservations observations;
  LiteralContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  auto& source = *monograph;

  Dynamic::Bytes integer =
      render_rejection(domain, source, "18446744073709551616"_view);
  Dynamic::Bytes negative =
      render_rejection(domain, source, "-9223372036854775809"_view);
  Dynamic::Bytes bytes = render_rejection(domain, source, "0x[F]"_view);
  Dynamic::Bytes operand = render_rejection(domain, source, "value"_view);

  EXPECT(contains(integer, "Unable to parse unsigned literal value."_view));
  EXPECT(contains(negative, "Unable to parse signed literal value."_view));
  EXPECT(contains(bytes, "incomplete hexadecimal byte"_view));
  EXPECT(contains(bytes, "every byte has two digits"_view));
  EXPECT(contains(operand, "requires a supported literal operand"_view));
  EXPECT(contains(operand, "Use a string, byte array"_view));
}

PERIMORTEM_UNIT_TEST(LiteralTests, malformed_ranges) {
  Allocator::Arena domain;
  LiteralObservations observations;
  LiteralContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  auto& source = *monograph;

  EXPECT(rejects(domain, source, "18446744073709551616"_view));
  EXPECT(rejects(domain, source, "-9223372036854775809"_view));
  EXPECT(rejects(domain, source, "0x10000000000000000"_view));
  EXPECT(rejects(domain, source, "0x[F]"_view));
  EXPECT(rejects(domain, source, "0x[GG]"_view));
  EXPECT(rejects(domain, source, "value"_view));
}

PERIMORTEM_UNIT_TEST(LiteralTests, embedded_resolution) {
  Allocator::Arena domain;
  LiteralObservations observations;
  LiteralContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  auto& source = *monograph;
  View::Bytes table_backing = context.table.get_value();
  Errors errors;
  Tokenizer tokenizer(domain, "$[table] $[empty]"_view, "embedded.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  auto table = Library::Interpreter::Literal::parse(source, cursor);
  EXPECT(observations.table_seen);
  auto empty = Library::Interpreter::Literal::parse(source, cursor);
  EXPECT(observations.empty_seen);

  Errors postfix_errors;
  Tokenizer postfix_tokenizer(
      domain, "$[table]:[2, 4]"_view, "postfix-slice.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations postfix_associations(
      postfix_tokenizer.get_arena());
  Cursor postfix_cursor(
      postfix_tokenizer, postfix_errors, postfix_associations);
  auto postfix_base =
      Library::Interpreter::Literal::parse(source, postfix_cursor);

  ASSERT(table && empty && postfix_base);
  ASSERT(table->is<Library::Language::Constants::Bytes>());
  ASSERT(empty->is<Library::Language::Constants::Bytes>());
  ASSERT(postfix_base->is<Library::Language::Constants::Bytes>());
  const auto& table_bytes =
      static_cast<const Library::Language::Constants::Bytes&>(*table);
  const auto& postfix_bytes =
      static_cast<const Library::Language::Constants::Bytes&>(*postfix_base);
  EXPECT(table_bytes.get_value().get_data() == table_backing.get_data());
  EXPECT(postfix_bytes.get_value().get_data() == table_backing.get_data());
  EXPECT_TEXT(table_bytes.get_value(), "0123456789"_view);
  EXPECT(
      static_cast<const Library::Language::Constants::Bytes&>(*empty)
          .get_value()
          .is_empty());
  EXPECT_TEXT(postfix_bytes.get_value(), "0123456789"_view);
  EXPECT(cursor.matches(Code::Type::Terminal));
  EXPECT(postfix_cursor.matches(Code::Type::ValueAccessOp));
  EXPECT(errors.is_empty());
  EXPECT(postfix_errors.is_empty());
  EXPECT(rejects(domain, source, "$[missing]"_view));
  EXPECT(rejects(domain, source, "$[other]"_view));
}

PERIMORTEM_UNIT_TEST(LiteralTests, contextual_error) {
  Allocator::Arena domain;
  Allocator::Arena render_arena;
  LiteralObservations observations;
  LiteralContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  auto& source = *monograph;
  Errors errors;
  Tokenizer tokenizer(domain, "\n$[error]"_view, "resource-error.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  auto parsed = Library::Interpreter::Literal::parse(source, cursor);
  View::Bytes rendered = errors.render_message(render_arena, 0);

  EXPECT_NOT(parsed);
  EXPECT(observations.error_seen);
  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(
      Algorithm::search(rendered, "resource-error.ttx:2:1"_view) != Count(-1));
  EXPECT(
      Algorithm::search(rendered, "Package supplied literal failure."_view) !=
      Count(-1));
  EXPECT(Algorithm::search(rendered, "^-------"_view) != Count(-1));
}
