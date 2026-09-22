// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/model/image.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/library.hpp"

#include "tetrodotoxin/model/execution/function.hpp"
#include "tetrodotoxin/model/type/primitives/u32.hpp"
#include "tetrodotoxin/source/contents/memory.hpp"
#include "tetrodotoxin/source/declaration.hpp"
#include "tetrodotoxin/source/dialect.hpp"
#include "tetrodotoxin/source/lexical/tokenization.hpp"
#include "ttx/concept/answers/none.hpp"
#include "ttx/concept/modules/module.hpp"
#include "validation/unit_tests/tetrodotoxin/dialect/fixtures/lifetime.h"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Concept::Modules;
using namespace Ttx::Data::Form;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Semantic::Ownership;

static Validation::Harness MonographTests = {.name = "Source::Monograph"_view};
static const auto library_path =
    ".bin/bin/validation/unit_tests/tetrodotoxin/dialect/liblibrary.so"_view;
static const auto nesting_path =
    ".bin/bin/validation/unit_tests/tetrodotoxin/dialect/libnesting.so"_view;

// Fixture setup requires these capabilities. Expected failures are inspected
// in the individual tests, instead of being folded into this extraction.
template <typename Value, typename Failure>
static auto required(Utility::Result<Value, Failure> result) -> Value {
  return result.visit(
      [](auto& value) -> Value {
        if constexpr (__is_reference(Value)) {
          return value;
        } else {
          return Core::Data::take(value);
        }
      },
      [](const Failure& failure) -> Value {
        if constexpr (__is_same(Failure, Core::View::Bytes)) {
          Core::Diagnostics::Log::fatal(failure);
        } else {
          Core::Diagnostics::Log::fatal("Monograph fixture setup failed."_view);
        }
      });
}

static auto load(
    Core::View::Bytes path,
    monograph_lifetime& lifetime,
    Memory::Allocator::Arena& errors) -> Module {
  auto module = required(Module::load(path, errors));
  // Observe this test module's unload without keeping an extra OS-library
  // reference alive. All actual discovery uses Module::open and UUID binding.
  auto inspection = required(System::Library::open(path, errors));
  auto* address =
      required(inspection.symbol("monograph_fixture_watch"_view, errors));
  reinterpret_cast<decltype(&monograph_fixture_watch)>(address)(&lifetime);
  return module;
}

struct MonographTypes {
  Model::Type::Primitives::U32 u32;
  auto get_data() const -> Core::View::Bytes { return {}; }
  auto resolve_concept(Core::View::Bytes name) const -> Abstract {
    return name == "U32"_view ? Abstract::provide(u32)
                              : Answers::None::get_none();
  }
};

static auto form(Core::View::Bytes text, Memory::Allocator::Arena& arena)
    -> const Representation& {
  const auto schema = Schema::range(
      Native<U8>::reference, text.get_size(), 1, text.get_size(), 1);
  return required(Representation::compile(schema, arena));
}

static auto tokenize(Abstract source) -> Publication {
  static const Source::Lexical::Tokenization provider;
  const auto policy =
      required(Abstract::provide(provider).bind<Source::Tokenization>());
  return required(policy.tokenize(source));
}

struct MonographInput {
  Memory::Allocator::Arena arena;
  MonographTypes types;
  Source::Contents::Memory observation;
  Publication tokens;
  Source::Lexical::Cursor cursor;

  explicit MonographInput(Core::View::Bytes text)
      : observation(text, form(text, arena)),
        tokens(tokenize(Abstract::provide(observation))),
        cursor(required(tokens.get_query().bind<Source::Lexical::Cursor>())) {}
};

struct MonographModules {
  Memory::Allocator::Arena errors;
  Module library;
  Module nesting;
  Module::Acquisition language;
  Module::Acquisition pair;

  MonographModules(monograph_lifetime& leaf, monograph_lifetime& parent)
      : library(load(library_path, leaf, errors)),
        nesting(load(nesting_path, parent, errors)),
        language(required(library.open())),
        pair(required(nesting.open(language.get_query()))) {}
};

PERIMORTEM_UNIT_TEST(MonographTests, nested_invocations) {
  static monograph_lifetime leaf = {};
  static monograph_lifetime parent = {};
  Memory::Dynamic::Vector<Terminal::Llvm::Execution> artifacts;
  {
    MonographModules modules(leaf, parent);
    // The same C dialect is selected again with a different child provider.
    // One outer invocation now retains two pairs, each retaining two Library
    // invocations. None of them is required to be the source root.
    auto outer = required(modules.nesting.open(modules.pair.get_query()));
    const auto dialect = required(outer.bind<Source::Dialect>());
    const auto text =
        "public a : func = [] -> [U32] { return 11; } "
        "public b : func = [] -> [U32] { return 12; } "
        "public c : func = [] -> [U32] { return 13; } "
        "public d : func = [] -> [U32] { return 14; }"_view;
    MonographInput input(text);
    auto publication = required(
        dialect.interpret(input.cursor, Abstract::provide(input.types)));
    EXPECT(input.cursor.matches(Source::Lexical::Code::Type::Terminal));
    EXPECT_EQ(leaf.live, U64(4));
    EXPECT_EQ(parent.live, U64(3));
    input.tokens.close();

    // Input traversal is gone. All result navigation stays at the encountered
    // policy: the parent provides provenance but does not become a Function.
    const auto root = required(publication.get_query().bind<Abstract>());
    EXPECT(root.resolve() == root);
    EXPECT(
        root.supports<Model::Execution::Function>() ==
        Binding::Status::Unsupported);
    const auto source = required(root.bind<Source::Declaration>());
    EXPECT(
        source.get_anchor()->get_source() ==
        Abstract::provide(input.observation));
    EXPECT_EQ(source.get_anchor()->get_extent().get_size(), text.get_size());
    EXPECT(
        root.resolve_concept("missing"_view).supports<Answers::None>() ==
        Binding::Status::Satisfied);

    Count children = 0;
    auto visit = [&](Core::View::Bytes route, Abstract child) {
      EXPECT(root.resolve_concept(route) == child);
      ++children;
    };
    root.visit_concepts(Abstract::Visitor(visit));
    EXPECT_EQ(children, Count(2));

    const Core::View::Bytes routes[] = {"left"_view, "right"_view};
    const Core::View::Bytes names[] = {"a"_view, "b"_view, "c"_view, "d"_view};
    Count index = 0;
    for (const auto group : routes) {
      for (const auto member : routes) {
        const auto subject =
            root.resolve_concept(group).resolve_concept(member);
        EXPECT(subject.get_data() == names[index++]);
        EXPECT(
            subject.supports<Model::Execution::Function>() ==
            Binding::Status::Satisfied);
        const auto authored = required(subject.bind<Source::Declaration>());
        EXPECT(
            authored.get_anchor()->get_source() ==
            Abstract::provide(input.observation));
        artifacts.emplace(
            required(Terminal::Llvm::Execution::compile(subject)));
      }
    }

    publication.close();
    EXPECT_EQ(leaf.live, U64(0));
    EXPECT_EQ(leaf.releases, U64(4));
    EXPECT_EQ(parent.live, U64(0));
    EXPECT_EQ(parent.releases, U64(3));
    EXPECT_EQ(parent.children_released, U64(6));
  }

  // Both dialect modules and all discovery owners have ended. Each terminal
  // owns the facts required for execution; it must not revisit a Monograph.
  EXPECT_EQ(leaf.unloaded, U64(1));
  EXPECT_EQ(parent.unloaded, U64(1));
  for (Count i = 0; i < artifacts.get_size(); ++i) {
    const auto& artifact = artifacts.get_view().get_data()[i];
    Validation::ModelTests::Image image(artifact);
    ASSERT(image.is_set());
    Ttx::Semantic::Realization::Invocation invocation;
    ASSERT(
        invocation.connect(
            image.get_query(), artifact.get_operation(), artifact.get_inputs(),
            artifact.get_outputs()) == Binding::Status::Satisfied);
    U32 value = 0;
    EXPECT(invocation.invoke(nullptr, &value) == Ttx::Data::Status::Success);
    EXPECT_EQ(value, U32(11 + i));
  }
}

PERIMORTEM_UNIT_TEST(MonographTests, forwarded_invocation) {
  static monograph_lifetime leaf = {};
  static monograph_lifetime parent = {};
  {
    MonographModules modules(leaf, parent);
    const auto forward = required(
        modules.pair.resolve_concept("forward"_view).bind<Source::Dialect>());
    MonographInput input("public forwarded : func = [] -> [] {}"_view);
    auto publication = required(
        forward.interpret(input.cursor, Abstract::provide(input.types)));
    input.tokens.close();
    const auto root = required(publication.get_query().bind<Abstract>());
    EXPECT(root.get_data() == "forwarded"_view);
    EXPECT(
        root.supports<Model::Execution::Function>() ==
        Binding::Status::Satisfied);
    EXPECT_EQ(parent.live, U64(0));
    EXPECT_EQ(leaf.live, U64(1));
    publication.close();
    EXPECT_EQ(leaf.releases, U64(1));
    EXPECT_EQ(parent.releases, U64(0));
  }
  EXPECT_EQ(leaf.unloaded, U64(1));
  EXPECT_EQ(parent.unloaded, U64(1));
}

PERIMORTEM_UNIT_TEST(MonographTests, declined_composition) {
  static monograph_lifetime leaf = {};
  static monograph_lifetime parent = {};
  {
    MonographModules modules(leaf, parent);
    const Core::View::Bytes routes[] = {
      "reject"_view, "pending"_view, "unsupported"_view};
    const Binding::Failure failures[] = {
      Binding::Failure::Rejected, Binding::Failure::Pending,
      Binding::Failure::Unsupported};
    for (Count i = 0; i < 3; ++i) {
      const auto dialect = required(
          modules.pair.resolve_concept(routes[i]).bind<Source::Dialect>());
      MonographInput input("public accepted : func = [] -> [] {}"_view);
      dialect.interpret(input.cursor, Abstract::provide(input.types))
          .visit(
              [&](Publication&) { EXPECT(False); },
              [&](Binding::Failure failure) {
                EXPECT(failure == failures[i]);
              });
      // The completed child is released on every declined outcome. Consumed
      // progress belongs to Cursor even though no enclosing result is
      // published.
      EXPECT(input.cursor.matches(Source::Lexical::Code::Type::Terminal));
      EXPECT_EQ(leaf.live, U64(0));
      EXPECT_EQ(parent.live, U64(0));
      EXPECT_EQ(leaf.releases, U64(i + 1));
      EXPECT_EQ(parent.children_released, U64(i + 1));
    }
  }
  EXPECT_EQ(leaf.unloaded, U64(1));
  EXPECT_EQ(parent.unloaded, U64(1));
}

PERIMORTEM_UNIT_TEST(MonographTests, rejected_second_child) {
  static monograph_lifetime leaf = {};
  static monograph_lifetime parent = {};
  {
    MonographModules modules(leaf, parent);
    const auto dialect = required(modules.pair.bind<Source::Dialect>());
    MonographInput input(
        "public valid : func = [] -> [] {} "
        "public bad : func = [.a : U32, .a : U32] -> [] {}"_view);
    dialect.interpret(input.cursor, Abstract::provide(input.types))
        .visit(
            [&](Publication&) { EXPECT(False); },
            [&](Binding::Failure failure) {
              EXPECT(failure == Binding::Failure::Rejected);
            });
    EXPECT_EQ(leaf.releases, U64(1));
    EXPECT_EQ(parent.children_released, U64(1));
    EXPECT_EQ(parent.live, U64(0));
    ASSERT_EQ(input.cursor.get_error_count(), Count(1));
    EXPECT(
        input.cursor.get_error(0)->get_message() ==
        "Duplicate parameter name."_view);
  }
  EXPECT_EQ(leaf.unloaded, U64(1));
  EXPECT_EQ(parent.unloaded, U64(1));
}
