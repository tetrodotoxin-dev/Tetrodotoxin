// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/interpreter/expression.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/access/propagate.hpp"
#include "tetrodotoxin/library/language/access/unwrap.hpp"
#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/constants/result.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/flow/match.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/library/language/types/result.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness PropagationAccessTests = {
  .name = "Tetrodotoxin::Library::Language::Access::Propagation"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "OptionAccessTest"_view, "option-access.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto find_field(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Field&> {
  for (const Reference<Abstract>& candidate : composite.get_addressables()) {
    if (candidate.get().get_name() == name &&
        candidate.get().is<Language::Field>()) {
      return static_cast<const Language::Field&>(candidate.get());
    }
  }

  return {};
}

static auto find_function(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Function&> {
  for (const Reference<Abstract>& candidate : composite.get_callables()) {
    if (candidate.get().get_name() == name &&
        candidate.get().is<Language::Function>()) {
      return static_cast<const Language::Function&>(candidate.get());
    }
  }

  return {};
}

static auto parse_expression(
    Allocator::Arena& domain,
    const Abstract& context,
    Cursor& cursor) -> Option<Language::Model::Pack&> {
  auto parsed = Interpreter::Expression::parse(context, cursor);
  if (!parsed || !cursor.matches(Code::Type::Terminal)) {
    return {};
  }

  return *parsed;
}

static auto rejects_link(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return !monograph && !errors.is_empty() &&
         retains_library_source(workspace, "OptionAccessTest"_view);
}

static auto rejects_link(View::Bytes source, View::Bytes diagnostic) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  BAIL_IF(monograph || errors.is_empty());
  BAIL_IF(!retains_library_source(workspace, "OptionAccessTest"_view));

  Allocator::Arena render_arena;
  for (Count index = 0; index < errors.get_size(); index++) {
    auto rendered = errors.render_message(render_arena, index);
    if (Algorithm::search(rendered, diagnostic) != Count(-1)) {
      return True;
    }
  }

  return False;
}

static auto select_unsigned(const Language::Model::Pack& pack)
    -> Option<const Language::Constants::Unsigned&> {
  auto direct = pack.select_identity<Language::Constants::Unsigned>();
  if (direct) {
    return *direct;
  }

  auto entry = pack.get_layout().get_abstract(0);
  return entry.visit(
      []() -> Option<const Language::Constants::Unsigned&> { return {}; },
      [](const Abstract& selected) {
        return selected.select<Language::Constants::Unsigned>();
      });
}

PERIMORTEM_UNIT_TEST(PropagationAccessTests, receiving_type_fit) {
  static constexpr View::Bytes source =
      "// Receiving Type policy.\n"
      "dialect : Library;\n"
      "public Maybe : alias = Option[U64];\n"
      "private const present : Maybe = 7;"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto maybe = monograph->resolve_concept("Maybe"_view)
                   .resolve()
                   .select<Language::Types::Option>();
  ASSERT(maybe);
  auto element =
      maybe->get_element_type().select<Language::Model::Types::Unsigned>();
  ASSERT(element);

  Allocator::Arena fitted_arena;
  auto& empty = Language::Model::Pack::create_folded(
      fitted_arena,
      View::Vector<Tetrodotoxin::Source::PackReference<Language::Model::Pack>>());
  auto& value = Language::Constants::Unsigned::create_synthetic(
      fitted_arena, *element, U64(7));

  EXPECT(empty.fits_into(*maybe));
  EXPECT(value.fits_into(*maybe));
  EXPECT(element->accepts(value));
  EXPECT_NOT(element->create_fitted(fitted_arena, value));

  auto absent = maybe->create_fitted(fitted_arena, empty);
  auto present = maybe->create_fitted(fitted_arena, value);
  ASSERT(absent && present);
  auto absent_option = absent->select_identity<Language::Constants::Option>();
  auto present_option = present->select_identity<Language::Constants::Option>();
  ASSERT(absent_option && present_option);
  EXPECT(absent_option->get_kind() == Language::Types::Option::Kind::Absent);
  EXPECT_NOT(absent_option->get_payload());
  EXPECT(present_option->get_kind() == Language::Types::Option::Kind::Present);
  auto payload = present_option->get_payload();
  ASSERT(payload);
  EXPECT(&*payload == &value);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PropagationAccessTests, target_unwrap) {
  static constexpr View::Bytes source =
      "// Option target fitting and unwrap.\n"
      "dialect : Library;\n"
      "public Maybe : alias = Option[U64];\n"
      "private const absent : Maybe = ();\n"
      "private const present : Maybe = 7;\n"
      "private const absent_copy : Maybe = absent;\n"
      "private const present_copy : Maybe = present;\n"
      "private const fallback := absent!;\n"
      "private const selected := present!;\n"
      "public Options : struct {\n"
      "  public consume : func = [.value : Maybe] -> Maybe { return value; }\n"
      "}\n"
      "private absent_call := Options -> consume(());\n"
      "private present_call := Options -> consume(8);"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const auto& source_type = monograph->get_source();
  auto absent = find_field(source_type, "absent"_view);
  auto present = find_field(source_type, "present"_view);
  auto fallback = find_field(source_type, "fallback"_view);
  auto selected = find_field(source_type, "selected"_view);
  auto absent_copy = find_field(source_type, "absent_copy"_view);
  auto present_copy = find_field(source_type, "present_copy"_view);
  auto absent_call = find_field(source_type, "absent_call"_view);
  auto present_call = find_field(source_type, "present_call"_view);
  ASSERT(absent && present && absent_copy && present_copy);
  ASSERT(fallback && selected);
  ASSERT(absent_call && present_call);

  auto absent_constant = absent->get_constant();
  auto present_constant = present->get_constant();
  auto absent_copy_constant = absent_copy->get_constant();
  auto present_copy_constant = present_copy->get_constant();
  ASSERT(absent_constant && present_constant);
  ASSERT(absent_copy_constant && present_copy_constant);
  EXPECT(&*absent_copy_constant == &*absent_constant);
  EXPECT(&*present_copy_constant == &*present_constant);
  auto absent_option =
      absent_constant->select_identity<Language::Constants::Option>();
  auto present_option =
      present_constant->select_identity<Language::Constants::Option>();
  ASSERT(absent_option && present_option);
  EXPECT(absent_option->get_kind() == Language::Types::Option::Kind::Absent);
  EXPECT_NOT(absent_option->get_payload());
  EXPECT(present_option->get_kind() == Language::Types::Option::Kind::Present);
  auto payload = present_option->get_payload();
  ASSERT(payload);
  auto payload_value = select_unsigned(*payload);
  ASSERT(payload_value);
  EXPECT_EQ(payload_value->get_value(), U64(7));

  auto fallback_constant = fallback->get_constant();
  auto selected_constant = selected->get_constant();
  ASSERT(fallback_constant && selected_constant);
  auto fallback_value = select_unsigned(*fallback_constant);
  auto selected_value = select_unsigned(*selected_constant);
  ASSERT(fallback_value && selected_value);
  EXPECT_EQ(fallback_value->get_value(), U64(0));
  EXPECT_EQ(selected_value->get_value(), U64(7));

  EXPECT(absent_call->get_type().is<Language::Types::Option>());
  EXPECT(&absent_call->get_type() == &present_call->get_type());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PropagationAccessTests, edge_folding) {
  static constexpr View::Bytes source =
      "// Option propagation.\n"
      "dialect : Library;\n"
      "public Maybe : alias = Option[U64];\n"
      "private Nested : alias = Option[Maybe];\n"
      "private const absent : Maybe = ();\n"
      "private const present : Maybe = 7;\n"
      "private const nested_absent : Nested = ();\n"
      "public pass : func = [.value : Maybe] -> Maybe { return value?; }\n"
      "public stop : func = [.value : Maybe] -> [] {\n"
      "  state selected := value?;\n"
      "  return;\n"
      "}\n"
      "public chain : func = [] -> [] { return; }\n"
      "public return_absent : func = [] -> Maybe { return; }\n"
      "public return_present : func = [] -> Maybe { return 9; }\n"
      "public assign : func = [] -> Maybe {\n"
      "  state value : Maybe = ();\n"
      "  value = 4;\n"
      "  value = ();\n"
      "  return value;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto pass = find_function(monograph->get_source(), "pass"_view);
  auto stop = find_function(monograph->get_source(), "stop"_view);
  auto chain = find_function(monograph->get_source(), "chain"_view);
  ASSERT(pass && pass->get_body());
  ASSERT(stop && stop->get_body());
  ASSERT(chain && chain->get_body());

  // Parsing against completed Function bodies exposes the exact access graph
  // without adding a test only query to Return or Local.
  Allocator::Arena domain;
  Errors expression_errors;
  Tokenizer pass_tokenizer(domain, "value?"_view, "option-expression.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations pass_associations(pass_tokenizer.get_arena());
  Cursor pass_cursor(pass_tokenizer, expression_errors, pass_associations);
  Tokenizer stop_tokenizer(domain, "value?"_view, "option-expression.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations stop_associations(stop_tokenizer.get_arena());
  Cursor stop_cursor(stop_tokenizer, expression_errors, stop_associations);
  auto pass_pack = parse_expression(domain, *monograph, pass_cursor);
  auto stop_pack = parse_expression(domain, *monograph, stop_cursor);
  auto pass_propagate = pass_pack.visit(
      []() -> Option<Language::Access::Propagate&> { return {}; },
      [](Language::Model::Pack& selected) {
        return selected.select_identity<Language::Access::Propagate>();
      });
  auto stop_propagate = stop_pack.visit(
      []() -> Option<Language::Access::Propagate&> { return {}; },
      [](Language::Model::Pack& selected) {
        return selected.select_identity<Language::Access::Propagate>();
      });
  ASSERT(pass_propagate && stop_propagate);

  const Language::Model::Pack* pass_receiver = &pass_propagate->get_receiver();
  const Language::Model::Pack* pass_empty = &pass_propagate->get_escape();
  ASSERT(
      pass_propagate->link(pass_cursor, *pass->get_body(), pass->get_host()));
  ASSERT(
      pass_propagate->link(pass_cursor, *pass->get_body(), pass->get_host()));
  ASSERT(
      stop_propagate->link(stop_cursor, *stop->get_body(), stop->get_host()));
  ASSERT(
      stop_propagate->link(stop_cursor, *stop->get_body(), stop->get_host()));
  EXPECT(&pass_propagate->get_receiver() == pass_receiver);
  EXPECT(&pass_propagate->get_escape() == pass_empty);
  EXPECT(pass_propagate->get_escape().get_layout().is_empty());
  EXPECT(stop_propagate->get_escape().get_layout().is_empty());
  EXPECT(pass_propagate->get_escape().fits(pass->get_results()));
  EXPECT(stop_propagate->get_escape().fits(stop->get_results()));
  EXPECT(pass_propagate->get_escape().is_complete());
  auto option = pass_propagate->get_receiver()
                    .get_type()
                    .resolve()
                    .select<Language::Types::Option>();
  ASSERT(option);
  EXPECT(&pass_propagate->get_type() == &option->get_element_type());
  EXPECT(&stop_propagate->get_type() == &option->get_element_type());
  pass_propagate->finalize(pass_cursor);
  pass_propagate->finalize(pass_cursor);

  Bool dynamic_fold_succeeded = False;
  Option<Language::Model::Pack&> dynamic_folded;
  pass_propagate->fold().visit(
      [&](const Option<Language::Model::Pack&>& selected) {
        dynamic_fold_succeeded = True;
        dynamic_folded = selected;
      },
      [](const Language::Expression::Error&) {});
  EXPECT(dynamic_fold_succeeded);
  EXPECT_NOT(dynamic_folded);

  // Presence can replace the node with exact element flow. Absence keeps the
  // authored branch because an empty Pack cannot replace a one value output.
  Tokenizer present_tokenizer(
      domain, "present?"_view, "option-expression.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations present_associations(
      present_tokenizer.get_arena());
  Cursor present_cursor(
      present_tokenizer, expression_errors, present_associations);
  Tokenizer absent_tokenizer(
      domain, "absent?"_view, "option-expression.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations absent_associations(absent_tokenizer.get_arena());
  Cursor absent_cursor(
      absent_tokenizer, expression_errors, absent_associations);
  auto present_pack = parse_expression(domain, *monograph, present_cursor);
  auto absent_pack = parse_expression(domain, *monograph, absent_cursor);
  auto present_propagate = present_pack.visit(
      []() -> Option<Language::Access::Propagate&> { return {}; },
      [](Language::Model::Pack& selected) {
        return selected.select_identity<Language::Access::Propagate>();
      });
  auto absent_propagate = absent_pack.visit(
      []() -> Option<Language::Access::Propagate&> { return {}; },
      [](Language::Model::Pack& selected) {
        return selected.select_identity<Language::Access::Propagate>();
      });
  ASSERT(present_propagate && absent_propagate);
  ASSERT(present_propagate->link(
      present_cursor, *pass->get_body(), pass->get_host()));
  ASSERT(absent_propagate->link(
      absent_cursor, *pass->get_body(), pass->get_host()));
  present_propagate->finalize(present_cursor);
  absent_propagate->finalize(absent_cursor);

  Bool present_fold_succeeded = False;
  Option<Language::Model::Pack&> present_folded;
  present_propagate->fold().visit(
      [&](const Option<Language::Model::Pack&>& selected) {
        present_fold_succeeded = True;
        present_folded = selected;
      },
      [](const Language::Expression::Error&) {});
  ASSERT(present_fold_succeeded && present_folded);
  auto present_value = select_unsigned(*present_folded);
  ASSERT(present_value);
  EXPECT_EQ(present_value->get_value(), U64(7));

  Bool absent_fold_succeeded = False;
  Option<Language::Model::Pack&> absent_folded;
  absent_propagate->fold().visit(
      [&](const Option<Language::Model::Pack&>& selected) {
        absent_fold_succeeded = True;
        absent_folded = selected;
      },
      [](const Language::Expression::Error&) {});
  EXPECT(absent_fold_succeeded);
  EXPECT_NOT(absent_folded);
  EXPECT(absent_propagate->get_escape().get_layout().is_empty());
  EXPECT(absent_propagate->get_escape().is_complete());

  // The outer unwrap retains Propagate as its receiver. An absent inner value
  // leaves the chain unfolded instead of evaluating the right suffix.
  Tokenizer chain_tokenizer(
      domain, "nested_absent?!"_view, "option-expression.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations chain_associations(chain_tokenizer.get_arena());
  Cursor chain_cursor(chain_tokenizer, expression_errors, chain_associations);
  auto chain_pack = parse_expression(domain, *monograph, chain_cursor);
  auto unwrap = chain_pack.visit(
      []() -> Option<Language::Access::Unwrap&> { return {}; },
      [](Language::Model::Pack& selected) {
        return selected.select_identity<Language::Access::Unwrap>();
      });
  ASSERT(unwrap);
  auto inner =
      unwrap->get_receiver().select_identity<Language::Access::Propagate>();
  ASSERT(inner);
  EXPECT(&unwrap->get_receiver() == &*inner);
  ASSERT(unwrap->link(chain_cursor, *chain->get_body(), chain->get_host()));
  ASSERT(unwrap->link(chain_cursor, *chain->get_body(), chain->get_host()));
  EXPECT(inner->get_escape().fits(chain->get_results()));
  unwrap->finalize(chain_cursor);
  unwrap->finalize(chain_cursor);

  Bool chain_fold_succeeded = False;
  Option<Language::Model::Pack&> chain_folded;
  unwrap->fold().visit(
      [&](const Option<Language::Model::Pack&>& selected) {
        chain_fold_succeeded = True;
        chain_folded = selected;
      },
      [](const Language::Expression::Error&) {});
  EXPECT(chain_fold_succeeded);
  EXPECT_NOT(chain_folded);

  EXPECT(expression_errors.is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PropagationAccessTests, propagation_fixture) {
  static constexpr View::Bytes path =
      "validation/data/ttx/library/propagation.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Tetrodotoxin::Library::Dialect workspace_toolchain_library;

  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);

  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "PropagationAcceptance"_view, path, *source);
  ASSERT(interpreted && interpreted->is<Language::Monograph>());
  auto& monograph = static_cast<Language::Monograph&>(*interpreted);
  EXPECT_TEXT(
      monograph.get_documentation().get_line(1),
      "Library propagation and sum Type acceptance."_view);

  EXPECT(
      &workspace.resolve_concept("PropagationAcceptance"_view) == &monograph);

  const auto& source_type = monograph.get_source();
  auto maybe = source_type.resolve_concept("Maybe"_view)
                   .resolve()
                   .select<Language::Types::Option>();
  auto session = source_type.resolve_concept("Session"_view)
                     .select<Language::Types::Object>();
  auto outcome = source_type.resolve_concept("Outcome"_view)
                     .resolve()
                     .select<Language::Types::Result>();
  ASSERT(maybe && session && outcome);
  EXPECT(&maybe->get_element_type() == &monograph.resolve_concept("U64"_view));
  EXPECT(&outcome->get_value_type() == &monograph.resolve_concept("U64"_view));
  EXPECT(&outcome->get_error_type() == &monograph.resolve_concept("Bool"_view));

  // Published Fields expose fitting and scalar defaults as retained Packs.
  auto absent = find_field(source_type, "absent"_view);
  auto present = find_field(source_type, "present"_view);
  auto defaulted = find_field(source_type, "defaulted"_view);
  auto selected = find_field(source_type, "selected"_view);
  auto absent_session = find_field(source_type, "absent_session"_view);
  auto first_session = find_field(source_type, "first_session"_view);
  auto second_session = find_field(source_type, "second_session"_view);
  auto failed = find_field(source_type, "failed"_view);
  auto succeeded = find_field(source_type, "succeeded"_view);
  auto default_result_field = find_field(source_type, "default_result"_view);
  ASSERT(absent && present && defaulted && selected);
  ASSERT(absent_session && first_session && second_session);
  ASSERT(failed && succeeded && default_result_field);

  auto absent_constant = absent->get_constant();
  auto present_constant = present->get_constant();
  auto absent_option = absent_constant.visit(
      []() -> Option<Language::Constants::Option&> { return {}; },
      [](Language::Model::Pack& value) {
        return value.select_identity<Language::Constants::Option>();
      });
  auto present_option = present_constant.visit(
      []() -> Option<Language::Constants::Option&> { return {}; },
      [](Language::Model::Pack& value) {
        return value.select_identity<Language::Constants::Option>();
      });
  ASSERT(absent_option && present_option);
  EXPECT(absent_option->get_kind() == Language::Types::Option::Kind::Absent);
  EXPECT_NOT(absent_option->get_payload());
  EXPECT(present_option->get_kind() == Language::Types::Option::Kind::Present);
  auto present_payload = present_option->get_payload();
  ASSERT(present_payload);
  auto present_value = select_unsigned(*present_payload);
  ASSERT(present_value);
  EXPECT_EQ(present_value->get_value(), U64(7));

  auto defaulted_constant = defaulted->get_constant();
  auto selected_constant = selected->get_constant();
  ASSERT(defaulted_constant && selected_constant);
  auto defaulted_value = select_unsigned(*defaulted_constant);
  auto selected_value = select_unsigned(*selected_constant);
  ASSERT(defaulted_value && selected_value);
  EXPECT_EQ(defaulted_value->get_value(), U64(0));
  EXPECT_EQ(selected_value->get_value(), U64(7));

  auto failed_constant = failed->get_constant();
  auto succeeded_constant = succeeded->get_constant();
  auto authored_default_constant = default_result_field->get_constant();
  auto failed_result = failed_constant.visit(
      []() -> Option<Language::Constants::Result&> { return {}; },
      [](Language::Model::Pack& value) {
        return Language::Constants::Result::select(value);
      });
  auto succeeded_result = succeeded_constant.visit(
      []() -> Option<Language::Constants::Result&> { return {}; },
      [](Language::Model::Pack& value) {
        return Language::Constants::Result::select(value);
      });
  auto authored_default_result = authored_default_constant.visit(
      []() -> Option<Language::Constants::Result&> { return {}; },
      [](Language::Model::Pack& value) {
        return Language::Constants::Result::select(value);
      });
  ASSERT(failed_result && succeeded_result && authored_default_result);
  EXPECT(failed_result->get_kind() == Language::Types::Result::Kind::Error);
  EXPECT(succeeded_result->get_kind() == Language::Types::Result::Kind::Value);
  auto succeeded_value = select_unsigned(succeeded_result->get_payload());
  ASSERT(succeeded_value);
  EXPECT_EQ(succeeded_value->get_value(), U64(11));
  auto authored_default_value =
      select_unsigned(authored_default_result->get_payload());
  ASSERT(authored_default_value);
  EXPECT_EQ(authored_default_value->get_value(), U64(0));

  Allocator::Arena result_default_domain;
  auto result_default = outcome->create_default(result_default_domain);
  ASSERT(result_default);
  auto created_default_result =
      Language::Constants::Result::select(*result_default);
  ASSERT(created_default_result);
  EXPECT(
      created_default_result->get_kind() ==
      Language::Types::Result::Kind::Value);
  auto default_result_value =
      select_unsigned(created_default_result->get_payload());
  ASSERT(default_result_value);
  EXPECT_EQ(default_result_value->get_value(), U64(0));

  auto absent_session_constant = absent_session->get_constant();
  auto session_option = absent_session_constant.visit(
      []() -> Option<Language::Constants::Option&> { return {}; },
      [](Language::Model::Pack& value) {
        return value.select_identity<Language::Constants::Option>();
      });
  ASSERT(session_option);
  EXPECT(session_option->get_kind() == Language::Types::Option::Kind::Absent);
  EXPECT(&session_option->get_type().get_element_type() == &*session);
  EXPECT(&first_session->get_type() == &*session);
  EXPECT(&second_session->get_type() == &*session);

  // Each authored unwrap keeps its own expression identity while both
  // receivers select the one absent Option Field.
  auto first_authored = first_session->get_initializer();
  auto second_authored = second_session->get_initializer();
  auto first_unwrap = first_authored.visit(
      []() -> Option<const Language::Access::Unwrap&> { return {}; },
      [](const Language::Model::Pack& value) {
        return value.select_identity<Language::Access::Unwrap>();
      });
  auto second_unwrap = second_authored.visit(
      []() -> Option<const Language::Access::Unwrap&> { return {}; },
      [](const Language::Model::Pack& value) {
        return value.select_identity<Language::Access::Unwrap>();
      });
  ASSERT(first_unwrap && second_unwrap);
  EXPECT(&*first_unwrap != &*second_unwrap);
  EXPECT(&first_unwrap->get_type() == &*session);
  EXPECT(&second_unwrap->get_type() == &*session);
  EXPECT(&first_unwrap->get_receiver().get_result() == &*absent_session);
  EXPECT(&second_unwrap->get_receiver().get_result() == &*absent_session);

  // Object defaults allocate independent Initializer and state Pack graphs
  // for each request.
  Allocator::Arena default_domain;
  const auto& session_type =
      static_cast<const Language::Model::Type&>(*session);
  auto first_default = session_type.create_default(default_domain);
  auto second_default = session_type.create_default(default_domain);
  ASSERT(first_default && second_default);
  EXPECT(&*first_default != &*second_default);
  auto first_initializer =
      first_default->select_identity<Language::Expressions::Initializer>();
  auto second_initializer =
      second_default->select_identity<Language::Expressions::Initializer>();
  ASSERT(first_initializer && second_initializer);
  EXPECT(&first_initializer->get_type() == &*session);
  EXPECT(&second_initializer->get_type() == &*session);
  auto first_values = first_initializer->get_completed_values();
  auto second_values = second_initializer->get_completed_values();
  ASSERT(first_values && second_values);
  EXPECT(&*first_values != &*second_values);
  ASSERT_EQ(first_values->get_layout().get_size(), Count(1));
  ASSERT_EQ(second_values->get_layout().get_size(), Count(1));
  auto first_count = first_values->get_layout().get_abstract(0);
  auto second_count = second_values->get_layout().get_abstract(0);
  ASSERT(first_count && second_count);
  EXPECT(&*first_count != &*second_count);

  // Published Callables retain both propagation result shapes and the complete
  // Option match split.
  auto propagate = find_function(source_type, "propagate"_view);
  auto stop = find_function(source_type, "stop"_view);
  auto choose = find_function(source_type, "choose"_view);
  auto propagate_result = find_function(source_type, "propagate_result"_view);
  auto require = find_function(source_type, "require"_view);
  auto select_error = find_function(source_type, "select_error"_view);
  ASSERT(propagate && stop && choose && choose->get_body());
  ASSERT(propagate_result && propagate_result->get_body());
  ASSERT(require && require->get_body() && select_error);
  ASSERT_EQ(propagate->get_results().get_size(), Count(1));
  EXPECT(stop->get_results().is_empty());
  auto propagated_result = propagate->get_results().get_abstract(0);
  ASSERT(propagated_result);
  EXPECT(&propagated_result->resolve() == &*maybe);

  auto result_statements = propagate_result->get_body()->get_statements();
  ASSERT_EQ(result_statements.get_size(), Count(1));
  auto result_return = result_statements.get_data()[0]
                           .get_root()
                           .select<Language::Flow::Return>();
  ASSERT(result_return);
  auto result_propagation =
      result_return->get_pack().select_identity<Language::Access::Propagate>();
  ASSERT(result_propagation);
  EXPECT(
      &result_propagation->get_type().resolve() == &outcome->get_value_type());
  EXPECT(
      &result_propagation->get_escape().get_type().resolve() ==
      &outcome->get_error_type());
  EXPECT(
      result_propagation->get_escape().fits(propagate_result->get_results()));

  auto require_statements = require->get_body()->get_statements();
  ASSERT_EQ(require_statements.get_size(), Count(1));
  auto flag_propagation = require_statements.get_data()[0]
                              .get_root()
                              .select<Language::Access::Propagate>();
  ASSERT(flag_propagation);
  EXPECT(
      &flag_propagation->get_type().resolve() ==
      &monograph.resolve_concept("Bool"_view));
  EXPECT(flag_propagation->get_escape().get_layout().is_empty());

  auto statements = choose->get_body()->get_statements();
  ASSERT_EQ(statements.get_size(), Count(1));
  auto match =
      statements.get_data()[0].get_root().select<Language::Flow::Match>();
  ASSERT(match);
  ASSERT_EQ(match->get_case_count(), Count(1));
  auto case_kind = match->get_case_kind(0);
  ASSERT(case_kind);
  EXPECT(*case_kind == Language::Flow::Match::CaseKind::Value);
  auto payload = match->get_case_payload(0);
  ASSERT(payload);
  EXPECT(&payload->get_type() == &monograph.resolve_concept("U64"_view));
  ASSERT(match->get_case_body(0));
  ASSERT(match->get_default());

  const Language::Field* retained_first = &*first_session;
  const Language::Flow::Match* retained_match = &*match;
  Allocator::Arena repeated_domain;
  Tokenizer repeated_tokenizer(repeated_domain, *source, path);
  Tetrodotoxin::Source::Lexical::Associations repeated_associations(
      repeated_tokenizer.get_arena());
  Cursor repeated_cursor(repeated_tokenizer, errors, repeated_associations);
  ASSERT(monograph.link(repeated_cursor));
  ASSERT(monograph.finalize(repeated_cursor));
  auto repeated_first = find_field(source_type, "first_session"_view);
  ASSERT(repeated_first);
  EXPECT(&*repeated_first == retained_first);
  auto repeated_choose = find_function(source_type, "choose"_view);
  ASSERT(repeated_choose && repeated_choose->get_body());
  auto repeated_match = repeated_choose->get_body()
                            ->get_statements()
                            .get_data()[0]
                            .get_root()
                            .select<Language::Flow::Match>();
  ASSERT(repeated_match);
  EXPECT(&*repeated_match == retained_match);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PropagationAccessTests, exact_rejections) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Propagation receiver mismatch.\ndialect : Library; private invalid : func = [.value : U64] -> [] { state selected := value?; return; }"_view,
    "// Propagation result mismatch.\ndialect : Library; private invalid : func = [.value : Option[U64]] -> U64 { return value?; }"_view,
    "// Propagation multi result.\ndialect : Library; private invalid : func = [.value : Option[U64]] -> [Option[U64], Option[U64]] { return (value?, value); }"_view,
    "// Result error cannot disappear.\ndialect : Library; private Outcome : alias = Result[U64, Bool]; private invalid : func = [.value : Outcome] -> [] { state selected := value?; return; }"_view,
  }};
  static constexpr Static::Vector<View::Bytes, 4> diagnostics = {{
    "Postfix `?` requires a propagating value Type."_view,
    "Postfix `?` escape values do not fit the Function result Layout."_view,
    "Postfix `?` escape values do not fit the Function result Layout."_view,
    "Postfix `?` escape values do not fit the Function result Layout."_view,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    EXPECT(rejects_link(sources[index], diagnostics[index]));
  }
}

PERIMORTEM_UNIT_TEST(PropagationAccessTests, result_alternatives) {
  static constexpr View::Bytes identical =
      "// Identical Result alternatives.\n"
      "dialect : Library;\n"
      "private Unknown : alias = Result[U64, U64];"_view;
  static constexpr View::Bytes empty =
      "// Empty Result alternative.\n"
      "dialect : Library;\n"
      "private Unknown : alias = Result[Empty, Bool];\n"
      "private Empty : struct {}"_view;

  EXPECT(rejects_link(identical));
  EXPECT(rejects_link(
      empty,
      "Result alternatives must each produce one nonempty value Type."_view));
}

PERIMORTEM_UNIT_TEST(PropagationAccessTests, invalid_elimination) {
  static constexpr Static::Vector<View::Bytes, 5> sources = {{
    "// Unwrap receiver mismatch.\ndialect : Library; private invalid : func = [.value : Bool] -> Bool { return value!; }"_view,
    "// Option slice.\ndialect : Library; private invalid : func = [.value : Option[U64]] -> U64 { return value:[0]; }"_view,
    "// Some constructor.\ndialect : Library; private Maybe : alias = Option[U64]; private invalid := Maybe -> some(1);"_view,
    "// Empty constructor.\ndialect : Library; private Maybe : alias = Option[U64]; private invalid := Maybe -> empty();"_view,
    "// Option target mismatch.\ndialect : Library; private invalid : Option[U64] = false;"_view,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    EXPECT(rejects_link(sources[index]));
  }
}
