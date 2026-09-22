// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/or.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/interpreter/expression.hpp"
#include "tetrodotoxin/library/interpreter/operation.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/s8.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

static Harness LibraryOr = {
  .name = "Tetrodotoxin::Library::Language::Operations::Or"_view,
};

static auto link_operation(Operation& operation, const Abstract& context)
    -> Bool {
  Allocator::Arena transaction;
  Errors errors;
  Tokenizer tokenizer(transaction, {}, "<operation>"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  return operation.link(cursor, context);
}

class OrExpression : public Expression {
 public:
  OrExpression(View::Bytes name, const Abstract& type)
      : Expression({}), name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }

 private:
  View::Bytes name;
  const Abstract& type;
};

class OrFoldInput : public Operation {
 public:
  OrFoldInput(
      Allocator::Arena& domain,
      Model::Pack& input,
      Tetrodotoxin::Library::Language::Constant& result,
      Bool fails = False)
      : Operation(
            domain,
            Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 1>{{input}},
            {}),
        result(result),
        fails(fails) {}

  auto get_name() const -> View::Bytes override { return "Or input"_view; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto get_evaluations() const -> Count { return evaluations; }

 protected:
  auto evaluate_constants(Allocator::Arena&) -> Result<
      Option<Tetrodotoxin::Library::Language::Constant&>,
      Expression::Error> override {
    evaluations++;
    if (fails) {
      return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
    }

    return result;
  }

  auto select_type(const Abstract& context) const
      -> Option<const Model::Type&> override {
    return context.resolve_concept("Bool"_view).select<Model::Type>();
  }

 private:
  Tetrodotoxin::Library::Language::Constant& result;
  Bool fails;
  Count evaluations = 0;
};

static auto selected(
    const Result<Option<Model::Pack&>, Expression::Error>& result)
    -> Option<Tetrodotoxin::Library::Language::Constant&> {
  return result.visit(
      [](const Option<Model::Pack&>& folded)
          -> Option<Tetrodotoxin::Library::Language::Constant&> {
        return folded.visit(
            []() -> Option<Tetrodotoxin::Library::Language::Constant&> {
              return {};
            },
            [](Model::Pack& expression)
                -> Option<Tetrodotoxin::Library::Language::Constant&> {
              return expression
                  .select_identity<Tetrodotoxin::Library::Language::Constant>();
            });
      },
      [](const Expression::Error&)
          -> Option<Tetrodotoxin::Library::Language::Constant&> { return {}; });
}

static auto reports(
    const Result<Option<Model::Pack&>, Expression::Error>& result,
    Expression::Error::Type expected,
    const Abstract& origin) -> Bool {
  return result.visit(
      [](const Option<Model::Pack&>&) { return False; },
      [&](const Expression::Error& error) {
        return error.get_type() == expected && &error.get_subject() == &origin
                   ? True
                   : False;
      });
}

static auto is_dynamic(
    const Result<Option<Model::Pack&>, Expression::Error>& result) -> Bool {
  return result.visit(
      [](const Option<Model::Pack&>& folded) { return !folded ? True : False; },
      [](const Expression::Error&) { return False; });
}

static auto matches_anchor(
    const Expression& expression,
    View::Bytes source,
    View::Bytes focus,
    View::Bytes span) -> Bool {
  return expression.get_anchor().visit(
      []() { return False; },
      [&](const Anchor& anchor) {
        return anchor.get_token().caculate_text(source) == focus &&
                       anchor.get_span().caculate_text(source) == span
                   ? True
                   : False;
      });
}

PERIMORTEM_UNIT_TEST(LibraryOr, exact_type_and_edges) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::Boolean distinct_bool;
  Types::S8 s8;
  OrExpression canonical_left(
      "canonical left"_view, resolve_library_flag(source));
  OrExpression canonical_right(
      "canonical right"_view, resolve_library_flag(source));
  OrExpression distinct("distinct"_view, distinct_bool);
  OrExpression signed_value("signed"_view, s8);
  OrExpression invalid("invalid"_view, Unknown::get_unknown());
  auto& canonical =
      Operations::Or::create_synthetic(domain, canonical_left, canonical_right);
  auto& distinct_left =
      Operations::Or::create_synthetic(domain, distinct, canonical_right);
  auto& distinct_right =
      Operations::Or::create_synthetic(domain, canonical_left, distinct);
  auto& distinct_pair =
      Operations::Or::create_synthetic(domain, distinct, distinct);
  auto& signed_operation =
      Operations::Or::create_synthetic(domain, canonical_left, signed_value);
  auto& invalid_operation =
      Operations::Or::create_synthetic(domain, invalid, canonical_right);

  EXPECT(canonical.get_type().resolve().is<Unknown>());
  EXPECT_NOT(canonical.get_anchor());
  EXPECT(link_operation(canonical, source));
  EXPECT_NOT(link_operation(distinct_left, source));
  EXPECT_NOT(link_operation(distinct_right, source));
  EXPECT(link_operation(distinct_pair, source));
  EXPECT_NOT(link_operation(signed_operation, source));
  EXPECT_NOT(link_operation(invalid_operation, source));

  EXPECT(&canonical.get_type() == &resolve_library_flag(source));
  EXPECT(&distinct_pair.get_type() == &distinct_bool);
  EXPECT(is_dynamic(canonical.fold()));
  EXPECT(is_dynamic(distinct_pair.fold()));
  EXPECT(distinct_left.get_type().resolve().is<Unknown>());
  EXPECT(distinct_right.get_type().resolve().is<Unknown>());
  EXPECT(signed_operation.get_type().resolve().is<Unknown>());
  EXPECT(invalid_operation.get_type().resolve().is<Unknown>());
}

PERIMORTEM_UNIT_TEST(LibraryOr, truth_table) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::Boolean flag_type;
  auto& true_value = Constants::True::create_synthetic(domain, flag_type);
  auto& false_value = Constants::False::create_synthetic(domain, flag_type);
  auto& true_true =
      Operations::Or::create_synthetic(domain, true_value, true_value);
  auto& true_false =
      Operations::Or::create_synthetic(domain, true_value, false_value);
  auto& false_true =
      Operations::Or::create_synthetic(domain, false_value, true_value);
  auto& false_false =
      Operations::Or::create_synthetic(domain, false_value, false_value);

  EXPECT(link_operation(true_true, source));
  EXPECT(link_operation(true_false, source));
  EXPECT(link_operation(false_true, source));
  EXPECT(link_operation(false_false, source));

  auto both = selected(true_true.fold());
  auto left = selected(true_false.fold());
  auto right = selected(false_true.fold());
  auto neither = selected(false_false.fold());
  auto repeated = selected(false_false.fold());

  ASSERT(both && left && right && neither && repeated);
  EXPECT(both->is_identity<Constants::True>());
  EXPECT(left->is_identity<Constants::True>());
  EXPECT(right->is_identity<Constants::True>());
  EXPECT(neither->is_identity<Constants::False>());
  EXPECT(&*neither == &*repeated);
  EXPECT(&both->get_type() == &flag_type);
  EXPECT(&neither->get_type() == &flag_type);
}

PERIMORTEM_UNIT_TEST(LibraryOr, ordered_reachability) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  auto& true_value =
      Constants::True::create_synthetic(domain, resolve_library_flag(source));
  auto& false_value =
      Constants::False::create_synthetic(domain, resolve_library_flag(source));
  OrExpression dynamic("dynamic"_view, resolve_library_flag(source));
  OrFoldInput skipped_failure(domain, false_value, false_value, True);
  OrFoldInput reached_failure(domain, false_value, false_value, True);
  OrFoldInput dynamic_failure(domain, false_value, false_value, True);
  auto& skipped =
      Operations::Or::create_synthetic(domain, true_value, skipped_failure);
  auto& reached =
      Operations::Or::create_synthetic(domain, false_value, reached_failure);
  auto& dynamic_left =
      Operations::Or::create_synthetic(domain, dynamic, dynamic_failure);

  EXPECT(link_operation(skipped, source));
  EXPECT(link_operation(reached, source));
  EXPECT(link_operation(dynamic_left, source));

  auto skipped_result = selected(skipped.fold());

  ASSERT(skipped_result);
  EXPECT(skipped_result->is_identity<Constants::True>());
  EXPECT(skipped_failure.get_evaluations() == 0);
  EXPECT(reports(
      reached.fold(), Expression::Error::Type::InvalidConstant,
      reached_failure));
  EXPECT(reached_failure.get_evaluations() == 1);
  EXPECT(reports(
      dynamic_left.fold(), Expression::Error::Type::InvalidConstant,
      dynamic_failure));
  EXPECT(dynamic_failure.get_evaluations() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryOr, authored_parsing) {
  static constexpr View::Bytes success_source = "false or true"_view;
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Errors success_errors;
  Tokenizer success_tokens(domain, success_source, "or.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations success_associations(success_tokens.get_arena());
  Cursor success_cursor(success_tokens, success_errors, success_associations);
  Token success_left_token = success_cursor.consume();
  auto success_left_anchor =
      Anchor::create(success_left_token, Span(success_left_token));
  auto& success_left = Constants::False::create_authored(
      domain, resolve_library_flag(source), success_left_anchor);
  auto parsed = Interpreter::Operation::parse_binary(
      Code::Type::Or, source, success_cursor, success_left,
      Span(success_left_token));

  ASSERT(parsed && parsed->is_identity<Operations::Or>());
  EXPECT(parsed->get_type().resolve().is<Unknown>());
  EXPECT(matches_anchor(*parsed, success_source, "or"_view, success_source));
  EXPECT(success_cursor.matches(Code::Type::Terminal));
  EXPECT(success_errors.is_empty());
  EXPECT(parsed->link(success_cursor, source));

  Errors failure_errors;
  Tokenizer failure_tokens(domain, "false or"_view, "or.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations failure_associations(failure_tokens.get_arena());
  Cursor failure_cursor(failure_tokens, failure_errors, failure_associations);
  Token failure_left_token = failure_cursor.consume();
  auto failure_left_anchor =
      Anchor::create(failure_left_token, Span(failure_left_token));
  auto& failure_left = Constants::False::create_authored(
      domain, resolve_library_flag(source), failure_left_anchor);
  auto rejected = Interpreter::Operation::parse_binary(
      Code::Type::Or, source, failure_cursor, failure_left,
      Span(failure_left_token));

  EXPECT_NOT(rejected);
  EXPECT(failure_cursor.matches(Code::Type::Terminal));
  EXPECT_NOT(failure_errors.is_empty());

  Errors mismatch_errors;
  Tokenizer mismatch_tokens(domain, "false or 1"_view, "or.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations mismatch_associations(mismatch_tokens.get_arena());
  Cursor mismatch_cursor(
      mismatch_tokens, mismatch_errors, mismatch_associations);
  auto mismatch = Interpreter::Expression::parse(source, mismatch_cursor);

  ASSERT(mismatch && mismatch->is_identity<Operations::Or>());
  EXPECT(mismatch_errors.is_empty());
  EXPECT_NOT(mismatch->link(mismatch_cursor, source));
  EXPECT_EQ(mismatch_errors.get_size(), Count(1));
}
