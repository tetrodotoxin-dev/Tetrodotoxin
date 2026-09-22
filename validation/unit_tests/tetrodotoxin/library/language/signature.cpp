// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/signature.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "tetrodotoxin/source/layouts/addressable.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness SignatureTests = {
  .name = "Tetrodotoxin::Library::Language::Signature"_view,
};

static auto interpret(
    Workspace& workspace,
    Tetrodotoxin::Source::Lexical::Errors& errors,
    View::Bytes source) -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "SignatureTest"_view, "signature.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto find_function(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Function&> {
  for (Reference<Abstract> candidate : composite.get_callables()) {
    if (candidate.get().get_name() == name &&
        candidate.get().is<Language::Function>()) {
      return static_cast<const Language::Function&>(candidate.get());
    }
  }

  return {};
}

PERIMORTEM_UNIT_TEST(SignatureTests, signature_shape) {
  static constexpr View::Bytes source =
      "// Signature identity test.\n"
      "dialect : Library;\n"
      "public inspect : func = [.input : Bool,] -> [\n"
      "    .count : U64, .accepted : Bool,\n"
      "  ] { return (.count = 0, .accepted = false); }"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Tetrodotoxin::Source::Lexical::Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto function = find_function(monograph->get_source(), "inspect"_view);
  ASSERT(function);

  const Layout& parameters = function->get_parameters();
  const Layout& results = function->get_results();

  ASSERT_EQ(parameters.get_size(), Count(1));
  ASSERT(parameters.get_name(0));
  EXPECT_TEXT(*parameters.get_name(0), "input"_view);
  auto parameter = parameters.get_abstract(0);
  ASSERT(parameter && parameter->is<Tetrodotoxin::Source::Layouts::Addressable>());
  const auto& input =
      static_cast<const Tetrodotoxin::Source::Layouts::Addressable&>(*parameter);
  EXPECT(&input.get_type() == &monograph->resolve_concept("Bool"_view));

  ASSERT_EQ(results.get_size(), Count(2));
  ASSERT(results.get_name(0) && results.get_name(1));
  EXPECT_TEXT(*results.get_name(0), "count"_view);
  EXPECT_TEXT(*results.get_name(1), "accepted"_view);
  EXPECT(&*results.get_abstract(0) == &monograph->resolve_concept("U64"_view));
  EXPECT(&*results.get_abstract(1) == &monograph->resolve_concept("Bool"_view));
  EXPECT(results.get_abstract(0)->is<Tetrodotoxin::Source::Type>());
  EXPECT(results.get_abstract(1)->is<Tetrodotoxin::Source::Type>());

  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SignatureTests, self_reference) {
  static constexpr View::Bytes source =
      "// Self reference result.\n"
      "dialect : Library;\n"
      "public Buffer : struct {\n"
      "  private state size : U64;\n"
      "  public clear : func = [self] -> self {\n"
      "    self.size = 0;\n"
      "  }\n"
      "}\n"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Tetrodotoxin::Source::Lexical::Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto buffer = monograph->get_source()
                    .resolve_concept("Buffer"_view)
                    .select<Language::Types::Composite>();
  ASSERT(buffer);
  auto clear = find_function(*buffer, "clear"_view);
  ASSERT(clear);

  auto parameter = clear->get_parameters().get_abstract(0);
  auto returned = clear->get_results().get_abstract(0);
  ASSERT(parameter && returned);
  EXPECT(parameter->is<Tetrodotoxin::Source::Layouts::Addressable>());
  EXPECT(&*parameter == &*returned);
  EXPECT(clear->get_self_result());
  EXPECT_NOT(clear->get_results().get_name(0));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SignatureTests, strict_descriptor) {
  static constexpr Static::Vector<View::Bytes, 7> rejected = {{
    "// Bare parameter.\ndialect : Library; private invalid : func = Bool -> [] {}"_view,
    "// Positional parameter.\ndialect : Library; private invalid : func = [Bool] -> [] {}"_view,
    "// Mixed parameter.\ndialect : Library; public Packet : struct { public value : Bool; private invalid : func = [self, Bool] -> [] {} }"_view,
    "// Duplicate parameter.\ndialect : Library; private invalid : func = [.value : Bool, .value : Bool] -> [] {}"_view,
    "// Value-label separator.\ndialect : Library; private invalid : func = [.value = Bool] -> [] {}"_view,
    "// Static self result.\ndialect : Library; private invalid : func = [] -> self { return; }"_view,
    "// Self mixed with value results.\ndialect : Library; public Packet : struct { private invalid : func = [self] -> [self, Bool] { return self; } }"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    Tetrodotoxin::Library::Dialect workspace_toolchain_library;
    auto workspace_toolchain =
        Validation::create_library_toolchain(workspace_toolchain_library);
    Workspace workspace(*workspace_toolchain);
    Tetrodotoxin::Source::Lexical::Errors errors;
    EXPECT_NOT(interpret(workspace, errors, rejected[i]));
    EXPECT_NOT(errors.is_empty());
    EXPECT(retains_library_source(workspace, "SignatureTest"_view));
  }
}
