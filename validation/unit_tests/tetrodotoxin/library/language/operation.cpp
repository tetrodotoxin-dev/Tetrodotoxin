// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operation.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/types/u64.hpp"
#include "tetrodotoxin/library/language/types/u8.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;
using namespace Validation;

static Harness LibraryOperation = {
  .name = "Tetrodotoxin::Library::Language::Operation"_view,
};

class OperationExpression : public Expression {
 public:
  OperationExpression(View::Bytes name, const Tetrodotoxin::Source::Type& type)
      : Expression({}), name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto get_type() const -> const Tetrodotoxin::Source::Type& override { return type; }

 private:
  View::Bytes name;
  const Tetrodotoxin::Source::Type& type;
};

class TestOperation : public Operation {
 public:
  TestOperation(
      Allocator::Arena& domain,
      View::Bytes name,
      const Model::Type& type,
      View::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>> inputs,
      Tetrodotoxin::Library::Language::Constant& result,
      Bool fails = False,
      Bool skips_after_first = False)
      : Operation(domain, inputs, {}),
        name(name),
        type(type),
        result(result),
        fails(fails),
        skips_after_first(skips_after_first) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto get_evaluations() const -> Count { return evaluations; }
  auto input_is(Count index, const Model::Pack& expected) const -> Bool {
    auto inputs = get_inputs();
    return index < inputs.get_size() &&
           &inputs.get_data()[index].get() == &expected;
  }
  auto input_missing(Count index) const -> Bool {
    return index >= get_inputs().get_size();
  }

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

  auto select_type(const Abstract&) const
      -> Option<const Model::Type&> override {
    return type;
  }

  auto reaches_next_input(
      Count folded_input,
      const Tetrodotoxin::Library::Language::Constant&) const -> Bool override {
    return !skips_after_first || folded_input != 0;
  }

 private:
  View::Bytes name;
  const Model::Type& type;
  Tetrodotoxin::Library::Language::Constant& result;
  Bool fails;
  Bool skips_after_first;
  Count evaluations = 0;
};

static auto link_operation(Operation& operation, const Abstract& context)
    -> Bool {
  Allocator::Arena transaction;
  Tetrodotoxin::Source::Lexical::Errors errors;
  Tetrodotoxin::Source::Lexical::Tokenizer tokenizer(transaction, {}, "<operation>"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Tetrodotoxin::Source::Lexical::Cursor cursor(tokenizer, errors, associations);
  return operation.link(cursor, context);
}

static auto selects(
    const Result<Option<Model::Pack&>, Expression::Error>& result,
    const Model::Pack& expected) -> Bool {
  return result.visit(
      [&](const Option<Model::Pack&>& selected) {
        return selected && &*selected == &expected ? True : False;
      },
      [](const Expression::Error&) { return False; });
}

static auto reports(
    const Result<Option<Model::Pack&>, Expression::Error>& result,
    Expression::Error::Type expected,
    const Abstract& expression) -> Bool {
  return result.visit(
      [](const Option<Model::Pack&>&) { return False; },
      [&](const Expression::Error& selected) {
        return selected.get_type() == expected &&
                       &selected.get_subject() == &expression
                   ? True
                   : False;
      });
}

static auto is_dynamic(
    const Result<Option<Model::Pack&>, Expression::Error>& result) -> Bool {
  return result.visit(
      [](const Option<Model::Pack&>& selected) {
        return !selected ? True : False;
      },
      [](const Expression::Error&) { return False; });
}

PERIMORTEM_UNIT_TEST(LibraryOperation, dynamic_inputs) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U64 type;
  OperationExpression ordinary("ordinary"_view, type);
  auto& constant = Constants::Unsigned::create_synthetic(domain, type, 1);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 2> inputs = {
    {ordinary, constant}};
  TestOperation operation(domain, "partial"_view, type, inputs, constant);

  EXPECT(operation.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(operation, source));
  EXPECT(link_operation(operation, source));
  EXPECT(&operation.get_type() == &type);

  auto first = operation.fold();
  auto second = operation.fold();

  EXPECT(is_dynamic(first));
  EXPECT(is_dynamic(second));
  EXPECT(operation.input_is(0, ordinary));
  EXPECT(operation.input_is(1, constant));
  EXPECT(operation.input_missing(2));
  EXPECT(operation.get_evaluations() == 0);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, partial_fold) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U64 type;
  OperationExpression ordinary("ordinary"_view, type);
  auto& input = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& folded = Constants::Unsigned::create_synthetic(domain, type, 2);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 1> child_inputs = {
    {input}};
  TestOperation child(domain, "child"_view, type, child_inputs, folded);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 2> parent_inputs = {
    {child, ordinary}};
  TestOperation parent(domain, "parent"_view, type, parent_inputs, folded);

  EXPECT(parent.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(parent, source));
  EXPECT(&parent.get_type() == &type);

  auto first = parent.fold();
  auto second = parent.fold();

  EXPECT(is_dynamic(first));
  EXPECT(is_dynamic(second));
  EXPECT(selects(child.fold(), folded));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(parent.get_evaluations() == 0);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, stable_result) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U64 type;
  auto& first_input = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& second_input = Constants::Unsigned::create_synthetic(domain, type, 2);
  auto& folded = Constants::Unsigned::create_synthetic(domain, type, 3);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 2> inputs = {{
    first_input,
    second_input,
  }};
  TestOperation operation(domain, "sum"_view, type, inputs, folded);

  EXPECT(operation.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(operation, source));
  EXPECT(&operation.get_type() == &type);

  auto first = operation.fold();
  auto second = operation.fold();

  EXPECT(selects(first, folded));
  EXPECT(selects(second, folded));
  EXPECT(operation.get_evaluations() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, child_failure) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U64 type;
  OperationExpression ordinary("ordinary"_view, type);
  auto& input = Constants::Unsigned::create_synthetic(domain, type, 1);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 1> child_inputs = {
    {input}};
  TestOperation child(domain, "child"_view, type, child_inputs, input, True);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 2> parent_inputs = {
    {child, ordinary}};
  TestOperation parent(domain, "parent"_view, type, parent_inputs, input);

  EXPECT(parent.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(parent, source));
  EXPECT(&parent.get_type() == &type);

  auto direct = child.fold();
  auto folding = parent.fold();
  auto repeated = parent.fold();

  EXPECT(reports(direct, Expression::Error::Type::InvalidConstant, child));
  EXPECT(reports(folding, Expression::Error::Type::InvalidConstant, child));
  EXPECT(reports(repeated, Expression::Error::Type::InvalidConstant, child));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(parent.get_evaluations() == 0);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, changed_result) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U64 expected_type;
  Types::U8 changed_type;
  auto& input = Constants::Unsigned::create_synthetic(domain, expected_type, 1);
  auto& changed =
      Constants::Unsigned::create_synthetic(domain, changed_type, 1);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 1> inputs = {{input}};
  TestOperation operation(
      domain, "changed"_view, expected_type, inputs, changed);

  EXPECT(operation.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(operation, source));
  EXPECT(&operation.get_type() == &expected_type);

  auto folded = operation.fold();

  EXPECT(
      reports(folded, Expression::Error::Type::ResultTypeMismatch, operation));
  EXPECT(operation.get_evaluations() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, retryable_query) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U64 type;
  auto& input = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& folded = Constants::Unsigned::create_synthetic(domain, type, 2);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 1> inputs = {{input}};
  TestOperation operation(domain, "retry"_view, type, inputs, folded);

  auto before = operation.fold();
  EXPECT(is_dynamic(before));
  EXPECT(operation.get_evaluations() == 0);

  ASSERT(link_operation(operation, source));
  auto after = operation.fold();
  EXPECT(selects(after, folded));
  EXPECT(operation.get_evaluations() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryOperation, reachability) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::U64 type;
  OperationExpression dynamic("dynamic"_view, type);
  auto& input = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& folded = Constants::Unsigned::create_synthetic(domain, type, 2);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 1> child_inputs = {
    {input}};
  TestOperation failing(
      domain, "failing"_view, type, child_inputs, folded, True);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 2> inputs = {
    {dynamic, failing}};
  TestOperation parent(
      domain, "parent"_view, type, inputs, folded, False, True);

  ASSERT(link_operation(parent, source));
  auto folded_result = parent.fold();

  EXPECT(reports(
      folded_result, Expression::Error::Type::InvalidConstant, failing));
  EXPECT(failing.get_evaluations() == 1);

  auto& first = Constants::Unsigned::create_synthetic(domain, type, 1);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 1> skipped_inputs = {
    {first}};
  TestOperation unreachable(
      domain, "unreachable"_view, type, skipped_inputs, folded, True);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 2> skipping_inputs = {
    {first, unreachable}};
  TestOperation skipping(
      domain, "skipping"_view, type, skipping_inputs, folded, False, True);

  ASSERT(link_operation(skipping, source));
  auto skipped_result = skipping.fold();

  EXPECT(selects(skipped_result, folded));
  EXPECT(unreachable.get_evaluations() == 0);
}
