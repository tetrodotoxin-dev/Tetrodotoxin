// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/interpreter/expression.hpp"
#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/slice.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/operations/add.hpp"
#include "tetrodotoxin/library/language/operations/and.hpp"
#include "tetrodotoxin/library/language/operations/divide.hpp"
#include "tetrodotoxin/library/language/operations/equal.hpp"
#include "tetrodotoxin/library/language/operations/greater.hpp"
#include "tetrodotoxin/library/language/operations/greater_equal.hpp"
#include "tetrodotoxin/library/language/operations/less.hpp"
#include "tetrodotoxin/library/language/operations/less_equal.hpp"
#include "tetrodotoxin/library/language/operations/modulo.hpp"
#include "tetrodotoxin/library/language/operations/multiply.hpp"
#include "tetrodotoxin/library/language/operations/negate.hpp"
#include "tetrodotoxin/library/language/operations/not.hpp"
#include "tetrodotoxin/library/language/operations/not_equal.hpp"
#include "tetrodotoxin/library/language/operations/or.hpp"
#include "tetrodotoxin/library/language/operations/range.hpp"
#include "tetrodotoxin/library/language/operations/subtract.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

static Harness ExpressionParserTests = {
  .name = "Tetrodotoxin::Library::Language::Expression parser"_view,
};

class ExpressionParserResource : public Tetrodotoxin::Language::Resource {
 public:
  ExpressionParserResource(Allocator::Arena& domain, View::Bytes value)
      : value(domain.proxy(value)) {}

  auto get_value() const -> View::Bytes override { return value; }

 private:
  View::Bytes value;
};

struct ExpressionParserObservations {
  Bool table_seen = False;
};

class ExpressionParserContext : public Abstract {
 public:
  ExpressionParserContext(
      Allocator::Arena& domain,
      ExpressionParserObservations& observations)
      : observations(observations), table(domain, "0123456789"_view) {}

  auto get_name() const -> View::Bytes override { return "Context"_view; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes route) const -> const Abstract& override {
    if (route == "$[table]"_view) {
      observations.table_seen = True;
      return table;
    }

    return Unknown::get_unknown();
  }

  ExpressionParserObservations& observations;
  ExpressionParserResource table;
};

static auto create_monograph(
    Allocator::Arena& domain,
    Library::Dialect& dialect,
    Abstract& context) -> Option<Library::Language::Monograph&> {
  Errors errors;
  Tokenizer tokenizer(domain, ""_view, "expression-source.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  Anchor source_anchor = Anchor::create(Span());
  auto interpretation = dialect.interpret(
      cursor, Tetrodotoxin::Source::Documentation::get_empty(), source_anchor, context);
  if (!interpretation || !interpretation->is<Library::Language::Monograph>() ||
      !errors.is_empty()) {
    return {};
  }

  return static_cast<Library::Language::Monograph&>(*interpretation);
}

static auto select_monograph(Option<Library::Language::Monograph&>& owner)
    -> Option<Library::Language::Monograph&> {
  return owner;
}

static auto parse_one(
    Allocator::Arena& domain,
    const Abstract& context,
    View::Bytes source,
    Errors& errors) -> Option<Library::Language::Model::Pack&> {
  Tokenizer tokenizer(domain, source, "expression-parser.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto parsed = Library::Interpreter::Expression::parse(context, cursor);
  if (parsed && !cursor.matches(Code::Type::Terminal)) {
    return {};
  }

  return parsed;
}

static auto link_one(
    Allocator::Arena& domain,
    const Abstract& root_context,
    Library::Language::Model::Pack& pack,
    View::Bytes source,
    Errors& errors) -> Bool {
  Tokenizer tokenizer(domain, source, "expression-parser.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  return pack.link(cursor, root_context);
}

static auto finalize_one(
    Allocator::Arena& domain,
    Library::Language::Model::Pack& pack,
    View::Bytes source,
    Errors& errors) -> void {
  Tokenizer tokenizer(domain, source, "expression-parser.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  pack.finalize(cursor);
}

static auto rejects_grammar(
    Allocator::Arena& domain,
    const Abstract& context,
    View::Bytes source) -> Bool {
  Errors errors;
  Tokenizer tokenizer(domain, source, "rejected-expression.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto parsed = Library::Interpreter::Expression::parse(context, cursor);
  return !parsed && !errors.is_empty();
}

template <typename selected_type>
static auto select(const Abstract& abstract) -> Option<const selected_type&> {
  return abstract.visit<selected_type>(
      [](const selected_type& selected) -> Option<const selected_type&> {
        return selected;
      },
      [](const Abstract&) -> Option<const selected_type&> { return {}; });
}

template <typename selected_type>
static auto select_pack(const Library::Language::Model::Pack& pack)
    -> Option<const selected_type&> {
  return pack.select_identity<selected_type>();
}

static auto matches_anchor(
    const Library::Language::Model::Pack& pack,
    View::Bytes source,
    View::Bytes focus,
    View::Bytes span) -> Bool {
  return pack.get_anchor().visit(
      []() { return False; },
      [&](const Anchor& anchor) -> Bool {
        return anchor.get_token().caculate_text(source) == focus &&
                       anchor.get_span().caculate_text(source) == span
                   ? True
                   : False;
      });
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, original_operation) {
  static constexpr View::Bytes source = "2 * 3"_view;
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph_owner = create_monograph(domain, dialect, context);
  auto monograph = select_monograph(monograph_owner);
  ASSERT(monograph);
  Errors errors;
  auto parsed = parse_one(domain, *monograph, source, errors);
  ASSERT(
      parsed && parsed->is_identity<Library::Language::Operations::Multiply>());
  auto expression = select_pack<Library::Language::Expression>(*parsed);
  ASSERT(expression);
  const Library::Language::Expression* identity = &*expression;

  EXPECT(&parsed->get_type() == &Unknown::get_unknown());
  EXPECT(matches_anchor(*parsed, source, "*"_view, source));
  ASSERT(link_one(domain, *monograph, *parsed, source, errors));
  ASSERT(link_one(domain, *monograph, *parsed, source, errors));
  EXPECT(&*expression == identity);
  EXPECT(&parsed->get_type() == &monograph->resolve_concept("U64"_view));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, delayed_type_check) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto mismatch_owner = create_monograph(domain, dialect, context);
  auto mismatch_graph = select_monograph(mismatch_owner);
  ASSERT(mismatch_graph);
  Errors mismatch_errors;
  auto mismatch =
      parse_one(domain, *mismatch_graph, "2 * true"_view, mismatch_errors);
  ASSERT(mismatch);
  EXPECT(mismatch->is_identity<Library::Language::Operations::Multiply>());
  EXPECT_NOT(link_one(
      domain, *mismatch_graph, *mismatch, "2 * true"_view, mismatch_errors));
  ASSERT_EQ(mismatch_errors.get_size(), Count(1));
  Allocator::Arena rendered;
  EXPECT(
      Algorithm::search(
          mismatch_errors.render_message(rendered, 0), "2 * true"_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, delayed_access_check) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto bounds_owner = create_monograph(domain, dialect, context);
  auto real_owner = create_monograph(domain, dialect, context);
  auto bounds_graph = select_monograph(bounds_owner);
  auto real_graph = select_monograph(real_owner);
  ASSERT(bounds_graph && real_graph);
  Errors bounds_errors;
  Errors real_errors;
  auto bounds =
      parse_one(domain, *bounds_graph, "\"abc\":[9]"_view, bounds_errors);
  auto real = parse_one(domain, *real_graph, "\"abc\":[1.0]"_view, real_errors);
  ASSERT(bounds && real);
  EXPECT(bounds->is_identity<Library::Language::Access::Slice>());
  EXPECT(real->is_identity<Library::Language::Access::Slice>());
  EXPECT(link_one(
      domain, *bounds_graph, *bounds, "\"abc\":[9]"_view, bounds_errors));
  EXPECT_NOT(
      link_one(domain, *real_graph, *real, "\"abc\":[1.0]"_view, real_errors));
  ASSERT_EQ(real_errors.get_size(), Count(1));
  EXPECT(bounds_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, postfix_span) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph_owner = create_monograph(domain, dialect, context);
  auto monograph = select_monograph(monograph_owner);
  ASSERT(monograph);
  Errors slice_errors;
  auto value =
      parse_one(domain, *monograph, "\"abcd\":[1, 2]"_view, slice_errors);
  ASSERT(value);
  EXPECT(value->is_identity<Library::Language::Access::Slice>());
  EXPECT(matches_anchor(
      *value, "\"abcd\":[1, 2]"_view, ":["_view, "\"abcd\":[1, 2]"_view));
  EXPECT(slice_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, prefix_spans) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph_owner = create_monograph(domain, dialect, context);
  auto monograph = select_monograph(monograph_owner);
  ASSERT(monograph);
  Errors negate_errors;
  Errors not_errors;
  auto negate = parse_one(domain, *monograph, "--8"_view, negate_errors);
  auto logical = parse_one(domain, *monograph, "!true"_view, not_errors);
  ASSERT(negate && logical);
  EXPECT(negate->is_identity<Library::Language::Operations::Negate>());
  EXPECT(matches_anchor(*negate, "--8"_view, "-"_view, "--8"_view));
  EXPECT(logical->is_identity<Library::Language::Operations::Not>());
  EXPECT(matches_anchor(*logical, "!true"_view, "!"_view, "!true"_view));
  EXPECT(negate_errors.is_empty());
  EXPECT(not_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, operation_anchors) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph_owner = create_monograph(domain, dialect, context);
  auto monograph = select_monograph(monograph_owner);
  ASSERT(monograph);
  Errors errors;

  auto divide = parse_one(domain, *monograph, "8 / 2"_view, errors);
  auto add = parse_one(domain, *monograph, "1 + 2"_view, errors);
  auto logical_and =
      parse_one(domain, *monograph, "true and false"_view, errors);
  auto logical_or = parse_one(domain, *monograph, "false or true"_view, errors);
  auto equal = parse_one(domain, *monograph, "1 == 1"_view, errors);
  auto greater = parse_one(domain, *monograph, "2 > 1"_view, errors);
  auto greater_equal = parse_one(domain, *monograph, "2 >= 1"_view, errors);
  auto less = parse_one(domain, *monograph, "1 < 2"_view, errors);
  auto less_equal = parse_one(domain, *monograph, "1 <= 2"_view, errors);
  auto modulo = parse_one(domain, *monograph, "8 % 3"_view, errors);
  auto multiply = parse_one(domain, *monograph, "2 * 3"_view, errors);
  auto negate = parse_one(domain, *monograph, "-8"_view, errors);
  auto logical = parse_one(domain, *monograph, "!true"_view, errors);
  auto not_equal = parse_one(domain, *monograph, "1 != 2"_view, errors);
  auto value = parse_one(domain, *monograph, "\"a\":[0]"_view, errors);
  auto subtract = parse_one(domain, *monograph, "2 - 1"_view, errors);
  auto range = parse_one(domain, *monograph, "1...4"_view, errors);

  ASSERT(
      divide && add && logical_and && logical_or && equal && greater &&
      greater_equal && less && less_equal);
  ASSERT(modulo && multiply && negate && logical && not_equal && value);
  ASSERT(subtract && range);
  EXPECT(matches_anchor(*divide, "8 / 2"_view, "/"_view, "8 / 2"_view));
  EXPECT(matches_anchor(*add, "1 + 2"_view, "+"_view, "1 + 2"_view));
  EXPECT(matches_anchor(
      *logical_and, "true and false"_view, "and"_view, "true and false"_view));
  EXPECT(matches_anchor(
      *logical_or, "false or true"_view, "or"_view, "false or true"_view));
  EXPECT(matches_anchor(*equal, "1 == 1"_view, "=="_view, "1 == 1"_view));
  EXPECT(matches_anchor(*greater, "2 > 1"_view, ">"_view, "2 > 1"_view));
  EXPECT(
      matches_anchor(*greater_equal, "2 >= 1"_view, ">="_view, "2 >= 1"_view));
  EXPECT(matches_anchor(*less, "1 < 2"_view, "<"_view, "1 < 2"_view));
  EXPECT(matches_anchor(*less_equal, "1 <= 2"_view, "<="_view, "1 <= 2"_view));
  EXPECT(matches_anchor(*modulo, "8 % 3"_view, "%"_view, "8 % 3"_view));
  EXPECT(matches_anchor(*multiply, "2 * 3"_view, "*"_view, "2 * 3"_view));
  EXPECT(matches_anchor(*negate, "-8"_view, "-"_view, "-8"_view));
  EXPECT(matches_anchor(*logical, "!true"_view, "!"_view, "!true"_view));
  EXPECT(matches_anchor(*not_equal, "1 != 2"_view, "!="_view, "1 != 2"_view));
  EXPECT(matches_anchor(*value, "\"a\":[0]"_view, ":["_view, "\"a\":[0]"_view));
  EXPECT(matches_anchor(*subtract, "2 - 1"_view, "-"_view, "2 - 1"_view));
  EXPECT(matches_anchor(*range, "1...4"_view, "..."_view, "1...4"_view));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, complete_range_rhs) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph_owner = create_monograph(domain, dialect, context);
  auto monograph = select_monograph(monograph_owner);
  ASSERT(monograph);

  EXPECT(rejects_grammar(domain, *monograph, "1..."_view));
  EXPECT(rejects_grammar(domain, *monograph, "1...2...3"_view));
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, nested_precedence) {
  static constexpr View::Bytes source = "2 * 3 - 4 == 2"_view;
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph_owner = create_monograph(domain, dialect, context);
  auto monograph = select_monograph(monograph_owner);
  ASSERT(monograph);
  Errors errors;
  auto parsed = parse_one(domain, *monograph, source, errors);
  ASSERT(parsed);
  auto equal = select_pack<Library::Language::Operations::Equal>(*parsed);
  ASSERT(equal);
  EXPECT(matches_anchor(*equal, source, "=="_view, source));
  ASSERT(link_one(domain, *monograph, *parsed, source, errors));
  finalize_one(domain, *parsed, source, errors);
  auto folded = equal->get_folded();
  ASSERT(folded);
  EXPECT(folded->is_identity<Library::Language::Constants::True>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, ordered_chain) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto invalid_chain_owner = create_monograph(domain, dialect, context);
  auto invalid_chain_graph = select_monograph(invalid_chain_owner);
  ASSERT(invalid_chain_graph);
  Errors errors;
  auto comparison_chain =
      parse_one(domain, *invalid_chain_graph, "1 < 2 <= 3"_view, errors);
  ASSERT(comparison_chain);
  EXPECT(comparison_chain
             ->is_identity<Library::Language::Operations::LessEqual>());
  EXPECT_NOT(link_one(
      domain, *invalid_chain_graph, *comparison_chain, "1 < 2 <= 3"_view,
      errors));
  EXPECT_NOT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, embedded_slice) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph_owner = create_monograph(domain, dialect, context);
  auto monograph = select_monograph(monograph_owner);
  ASSERT(monograph);
  Errors errors;
  auto parsed = parse_one(domain, *monograph, "$[table]:[2, 4]"_view, errors);
  ASSERT(parsed && parsed->is_identity<Library::Language::Access::Slice>());
  EXPECT(observations.table_seen);
  EXPECT(matches_anchor(
      *parsed, "$[table]:[2, 4]"_view, ":["_view, "$[table]:[2, 4]"_view));
  EXPECT(link_one(domain, *monograph, *parsed, "$[table]:[2, 4]"_view, errors));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, address_chain) {
  static constexpr View::Bytes source = "receiver.member.tail"_view;
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph_owner = create_monograph(domain, dialect, context);
  auto monograph = select_monograph(monograph_owner);
  ASSERT(monograph);
  Errors errors;
  auto parsed = parse_one(domain, *monograph, source, errors);
  ASSERT(parsed);
  auto outer = select_pack<Library::Language::Access::Address>(*parsed);
  ASSERT(outer);
  auto inner = outer->get_receiver()
                   .select_identity<Library::Language::Access::Address>();
  ASSERT(inner);

  EXPECT_TEXT(outer->get_name(), "tail"_view);
  EXPECT_TEXT(inner->get_name(), "member"_view);
  EXPECT(matches_anchor(*outer, source, "tail"_view, source));
  EXPECT(matches_anchor(*inner, source, "member"_view, "receiver.member"_view));
  EXPECT(inner->get_receiver()
             .is_identity<Library::Language::Expressions::Identifier>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, parenthesized_pack) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph_owner = create_monograph(domain, dialect, context);
  auto monograph = select_monograph(monograph_owner);
  ASSERT(monograph);
  Errors errors;

  auto scalar = parse_one(domain, *monograph, "(2)"_view, errors);
  auto empty = parse_one(domain, *monograph, "()"_view, errors);
  auto positional = parse_one(domain, *monograph, "(1, true)"_view, errors);
  auto named =
      parse_one(domain, *monograph, "(.right = true, .left = 1)"_view, errors);
  ASSERT(scalar && empty && positional && named);

  // Parentheses do not manufacture a carrier around one positional value.
  // Empty, multiple, and named values remain complete Pack flows instead
  // of being laundered through an eager aggregate Type.
  EXPECT(scalar->is_identity<Library::Language::Constant>());
  EXPECT_NOT(empty->is_identity<Library::Language::Expression>());
  EXPECT_NOT(positional->is_identity<Library::Language::Expression>());
  EXPECT_NOT(named->is_identity<Library::Language::Expression>());
  EXPECT_NOT(empty->is_complete());
  EXPECT_NOT(positional->is_complete());
  EXPECT_NOT(named->is_complete());

  ASSERT(link_one(domain, *monograph, *scalar, "(2)"_view, errors));
  ASSERT(link_one(domain, *monograph, *empty, "()"_view, errors));
  ASSERT(link_one(domain, *monograph, *positional, "(1, true)"_view, errors));
  ASSERT(link_one(
      domain, *monograph, *named, "(.right = true, .left = 1)"_view, errors));
  EXPECT(empty->is_complete());
  EXPECT(positional->is_complete());
  EXPECT(named->is_complete());
  EXPECT_EQ(empty->get_layout().get_size(), Count(0));
  EXPECT_EQ(positional->get_layout().get_size(), Count(2));
  EXPECT_EQ(named->get_layout().get_size(), Count(2));
  EXPECT(named->get_type().is<Unknown>());

  auto positional_first = positional->get_layout().get_abstract(0);
  auto positional_second = positional->get_layout().get_abstract(1);
  auto named_first = named->get_layout().get_abstract(0);
  auto named_second = named->get_layout().get_abstract(1);
  ASSERT(positional_first && positional_second && named_first && named_second);
  EXPECT(positional_first->is<Library::Language::Constant>());
  EXPECT(positional_second->is<Library::Language::Constant>());
  EXPECT(named_first->is<Library::Language::Constant>());
  EXPECT(named_second->is<Library::Language::Constant>());
  auto positional_name = positional->get_layout().get_name(0);
  auto named_first_name = named->get_layout().get_name(0);
  auto named_second_name = named->get_layout().get_name(1);
  EXPECT_NOT(positional_name);
  ASSERT(named_first_name);
  ASSERT(named_second_name);
  EXPECT_TEXT(*named_first_name, "right"_view);
  EXPECT_TEXT(*named_second_name, "left"_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, expression_packs) {
  static constexpr View::Bytes source = "(2) * (3)"_view;
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph_owner = create_monograph(domain, dialect, context);
  auto monograph = select_monograph(monograph_owner);
  ASSERT(monograph);
  Errors errors;

  auto parsed = parse_one(domain, *monograph, source, errors);
  ASSERT(
      parsed && parsed->is_identity<Library::Language::Operations::Multiply>());
  EXPECT(matches_anchor(*parsed, source, "*"_view, source));
  EXPECT(link_one(domain, *monograph, *parsed, source, errors));
  EXPECT(errors.is_empty());

  EXPECT(rejects_grammar(domain, *monograph, "(1, 2) + 3"_view));
  EXPECT(rejects_grammar(domain, *monograph, "1 + (2, 3)"_view));
  EXPECT(rejects_grammar(domain, *monograph, "(.x = 1) + 2"_view));
  EXPECT(rejects_grammar(domain, *monograph, "!(.x = true)"_view));
  EXPECT(rejects_grammar(domain, *monograph, "(1, 2):[0]"_view));
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, grammar_errors) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph_owner = create_monograph(domain, dialect, context);
  auto monograph = select_monograph(monograph_owner);
  ASSERT(monograph);

  EXPECT(rejects_grammar(domain, *monograph, "2 *"_view));
  EXPECT(rejects_grammar(domain, *monograph, "true and"_view));
  EXPECT(rejects_grammar(domain, *monograph, "false or"_view));
  EXPECT(rejects_grammar(domain, *monograph, "receiver."_view));
  EXPECT(rejects_grammar(domain, *monograph, "\"abc\":[]"_view));
  EXPECT(rejects_grammar(domain, *monograph, "\"abc\":[1,]"_view));
  EXPECT(rejects_grammar(domain, *monograph, "\"abc\":[1, 1"_view));
  EXPECT(rejects_grammar(domain, *monograph, "\"abc\":[1 1]"_view));

  Errors binary_errors;
  Errors prefix_errors;
  EXPECT_NOT(parse_one(domain, *monograph, "2 +"_view, binary_errors));
  EXPECT_NOT(parse_one(domain, *monograph, "!"_view, prefix_errors));
  ASSERT_EQ(binary_errors.get_size(), Count(1));
  ASSERT_EQ(prefix_errors.get_size(), Count(1));
  Allocator::Arena rendered;
  EXPECT(
      Algorithm::search(
          binary_errors.render_message(rendered, 0),
          "requires one right operand"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          prefix_errors.render_message(rendered, 0),
          "requires one operand"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, rejects_bitwise) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph_owner = create_monograph(domain, dialect, context);
  auto monograph = select_monograph(monograph_owner);
  ASSERT(monograph);
  Errors and_errors;
  Errors or_errors;

  EXPECT_NOT(parse_one(domain, *monograph, "true & false"_view, and_errors));
  EXPECT_NOT(parse_one(domain, *monograph, "false | true"_view, or_errors));
  EXPECT(and_errors.is_empty());
  EXPECT(or_errors.is_empty());
}
