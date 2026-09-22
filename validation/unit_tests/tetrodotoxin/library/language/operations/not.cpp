// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/not.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/s8.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Tetrodotoxin::Source;
using namespace Validation;

static Harness LibraryNot = {
  .name = "Tetrodotoxin::Library::Language::Operations::Not"_view,
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

class NotExpression : public Expression {
 public:
  NotExpression(View::Bytes name, const Abstract& type)
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
            [](Model::Pack& selected)
                -> Option<Tetrodotoxin::Library::Language::Constant&> {
              return selected
                  .select_identity<Tetrodotoxin::Library::Language::Constant>();
            });
      },
      [](const Expression::Error&)
          -> Option<Tetrodotoxin::Library::Language::Constant&> { return {}; });
}

PERIMORTEM_UNIT_TEST(LibraryNot, type_selection) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::Boolean distinct_bool;
  Types::S8 s8;
  NotExpression canonical("canonical"_view, resolve_library_flag(source));
  NotExpression distinct("distinct"_view, distinct_bool);
  NotExpression signed_value("signed"_view, s8);
  NotExpression unresolved("unresolved"_view, Unknown::get_unknown());
  auto& canonical_not = Operations::Not::create_synthetic(domain, canonical);
  auto& distinct_not = Operations::Not::create_synthetic(domain, distinct);
  auto& signed_not = Operations::Not::create_synthetic(domain, signed_value);
  auto& invalid_not = Operations::Not::create_synthetic(domain, unresolved);

  EXPECT(canonical_not.get_type().resolve().is<Unknown>());
  EXPECT_NOT(canonical_not.get_anchor());
  EXPECT(link_operation(canonical_not, source));
  EXPECT(link_operation(distinct_not, source));
  EXPECT(!link_operation(signed_not, source));
  EXPECT(!link_operation(invalid_not, source));

  auto canonical_result = selected(canonical_not.fold());

  EXPECT_NOT(canonical_result);
  EXPECT(&canonical_not.get_type() == &resolve_library_flag(source));
  EXPECT(&distinct_not.get_type() == &distinct_bool);
  EXPECT(signed_not.get_type().resolve().is<Unknown>());
  EXPECT(invalid_not.get_type().resolve().is<Unknown>());
}

PERIMORTEM_UNIT_TEST(LibraryNot, flag_protocol) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Types::Boolean storage;
  Types::Boolean other_storage;
  const Model::Types::Flag& protocol = storage;
  auto& active = Constants::True::create_synthetic(domain, protocol);
  auto& inactive = Constants::False::create_synthetic(domain, protocol);
  auto& other_active = Constants::True::create_synthetic(domain, other_storage);
  Static::Vector<Tetrodotoxin::Source::PackReference<Model::Pack>, 2> entries{
    {active, inactive}};
  auto& folded = Model::Pack::create_folded(domain, entries);
  auto active_validity = protocol.get_validity(folded);
  auto inactive_validity = protocol.get_validity(inactive);
  auto& inverse = Operations::Not::create_synthetic(domain, active);

  ASSERT(active_validity && inactive_validity);
  EXPECT(*active_validity);
  EXPECT_NOT(*inactive_validity);
  EXPECT_NOT(protocol.get_validity(other_active));
  ASSERT(link_operation(inverse, source));

  auto inverse_value = selected(inverse.fold());
  ASSERT(inverse_value);
  auto inverse_validity = protocol.get_validity(*inverse_value);
  ASSERT(inverse_validity);
  EXPECT_NOT(*inverse_validity);
  EXPECT(&inverse_value->get_type() == &protocol);
}

PERIMORTEM_UNIT_TEST(LibraryNot, canonical_folding) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  auto& true_value =
      Constants::True::create_synthetic(domain, resolve_library_flag(source));
  auto& false_value =
      Constants::False::create_synthetic(domain, resolve_library_flag(source));
  auto& complete_true = Constants::Flag::create_synthetic(
      domain, resolve_library_flag(source), True);
  auto& complete_false = Constants::Flag::create_synthetic(
      domain, resolve_library_flag(source), False);
  auto& true_not = Operations::Not::create_synthetic(domain, true_value);
  auto& false_not = Operations::Not::create_synthetic(domain, false_value);
  auto& complete_true_not =
      Operations::Not::create_synthetic(domain, complete_true);
  auto& complete_false_not =
      Operations::Not::create_synthetic(domain, complete_false);

  EXPECT(true_not.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(true_not, source));
  EXPECT(link_operation(false_not, source));
  EXPECT(link_operation(complete_true_not, source));
  EXPECT(link_operation(complete_false_not, source));

  auto true_result = selected(true_not.fold());
  auto false_result = selected(false_not.fold());
  auto complete_true_result = selected(complete_true_not.fold());
  auto complete_false_result = selected(complete_false_not.fold());

  ASSERT(
      true_result && false_result && complete_true_result &&
      complete_false_result);
  EXPECT(true_result->is_identity<Constants::False>());
  EXPECT(false_result->is_identity<Constants::True>());
  EXPECT(complete_true_result->is_identity<Constants::False>());
  EXPECT(complete_false_result->is_identity<Constants::True>());
  EXPECT(&true_result->get_type() == &resolve_library_flag(source));
  EXPECT(&false_result->get_type() == &resolve_library_flag(source));
}

PERIMORTEM_UNIT_TEST(LibraryNot, stable_folding) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  auto& true_value =
      Constants::True::create_synthetic(domain, resolve_library_flag(source));
  auto& child = Operations::Not::create_synthetic(domain, true_value);
  auto& parent = Operations::Not::create_synthetic(domain, child);

  EXPECT(parent.get_type().resolve().is<Unknown>());
  EXPECT(link_operation(parent, source));
  EXPECT(link_operation(parent, source));

  auto parent_result = selected(parent.fold());
  auto repeated_result = selected(parent.fold());

  ASSERT(parent_result && repeated_result);
  EXPECT(parent_result->is_identity<Constants::True>());
  EXPECT(&*parent_result == &*repeated_result);
  EXPECT(&parent_result->get_type() == &resolve_library_flag(source));
}
