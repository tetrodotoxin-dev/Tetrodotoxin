// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialect/library/dialect.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/model/image.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/library.hpp"

#include "tetrodotoxin/model/type/primitives/u32.hpp"
#include "tetrodotoxin/source/contents/memory.hpp"
#include "tetrodotoxin/source/declaration.hpp"
#include "tetrodotoxin/source/lexical/tokenization.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "ttx/concept/answers/none.hpp"
#include "validation/unit_tests/tetrodotoxin/model/fixtures/cursor.h"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Data::Form;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Semantic::Ownership;

static Validation::Harness DialectTests = {
  .name = "Dialect::Library::Publication"_view};
static constexpr System::Uuid operation(0x01a201fea3b44861, 0xa3f996cd34ae0c32);
static const Dialect::Library::Dialect language(operation);
static const Source::Lexical::Tokenization lexer;

struct DialectTypes {
  Model::Type::Primitives::U32 u32;
  auto get_data() const -> Core::View::Bytes { return {}; }
  auto resolve_concept(Core::View::Bytes name) const -> Abstract {
    return name == "U32"_view ? Abstract::provide(u32)
                              : Answers::None::get_none();
  }
};

static auto representation(
    Core::View::Bytes text,
    Memory::Allocator::Arena& arena) -> const Representation& {
  const auto schema = Schema::range(
      Native<U8>::reference, text.get_size(), 1, text.get_size(), 1);
  return Representation::compile(schema, arena)
      .visit(
          [](const Representation& form) -> const Representation& {
            return form;
          },
          [](Ttx::Data::Status) -> const Representation& {
            Core::Diagnostics::Log::fatal("Invalid test source form."_view);
          });
}

template <typename Function>
static auto symbol(
    const System::Library& module,
    Core::View::Bytes name,
    Memory::Allocator::Arena& arena) -> Function {
  return module.symbol(name, arena)
      .visit(
          [](void* value) { return reinterpret_cast<Function>(value); },
          [](Core::View::Bytes error) -> Function {
            Core::Diagnostics::Log::fatal(error);
          });
}

// Setup failures invalidate the fixture itself. Keeping this checked extraction
// local lets each example show its ownership sequence without nesting every
// successful acquisition inside another callback. Expected failures below still
// inspect the Result explicitly.
template <typename Value>
static auto required(Utility::Result<Value, Binding::Failure> result) -> Value {
  return result.visit(
      [](Value& value) -> Value { return Core::Data::take(value); },
      [](Binding::Failure) -> Value {
        Core::Diagnostics::Log::fatal(
            "Required Cursor fixture capability was unavailable."_view);
      });
}

template <typename Inspect>
static auto compile_foreign(Core::View::Bytes text, Inspect inspect)
    -> Utility::Result<Terminal::Llvm::Execution, Binding::Failure> {
  Memory::Allocator::Arena arena;
  DialectTypes types;
  const Source::Contents::Memory input(text, representation(text, arena));
  const auto origin = Abstract::provide(input);
  const Source::Lexical::Tokenizer classified(
      arena, text, "foreign.ttx"_view, origin);
  auto entries = arena.reserve<cursor_fixture_entry>(classified.get_size());
  for (Count i = 0; i < classified.get_size(); ++i) {
    const auto token = classified[i];
    entries.get_data()[i] = cursor_fixture_entry(
        static_cast<U8>(token.get_code().get_type()),
        classified.get_extent(token));
  }

  return System::Library::
      open(".bin/bin/validation/unit_tests/tetrodotoxin/model/libcursor_foreign.so"_view,
           arena)
          .visit(
              [&](System::Library& module)
                  -> Utility::Result<
                      Terminal::Llvm::Execution, Binding::Failure> {
                const auto open = symbol<decltype(&cursor_fixture_open)>(
                    module, "cursor_fixture_open"_view, arena);
                const auto interpret =
                    symbol<decltype(&cursor_fixture_interpret)>(
                        module, "cursor_fixture_interpret"_view, arena);
                const auto observations =
                    symbol<decltype(&cursor_fixture_observations)>(
                        module, "cursor_fixture_observations"_view, arena);
                U32 releases = 0;
                Publication source(open(
                    origin.get_abi(), text,
                    entries.get_data(), entries.get_size(), &releases));
                return source.get_query().bind<Source::Lexical::Cursor>().visit(
                    [&](Source::Lexical::Cursor cursor)
                        -> Utility::Result<
                            Terminal::Llvm::Execution, Binding::Failure> {
                      // The fixture receives classified input but implements
                      // every Cursor operation in C. Its descending locators
                      // cannot be used as token indices or source coordinates
                      // by the C++ dialect.
                      // The repeated observation and copied facade should use
                      // one foreign read. No provider lookup is needed to fork.
                      const auto before = observations(source.get_query());
                      const auto first = cursor.current();
                      const auto same = cursor.current();
                      auto branch = cursor;
                      if (first != same || branch.consume() != first) {
                        return Binding::Failure::Rejected;
                      }

                      if (cursor.get_index() != 0 || branch.get_index() != 1) {
                        return Binding::Failure::Rejected;
                      }

                      if (observations(source.get_query()) != before + 1) {
                        return Binding::Failure::Rejected;
                      }

                      if (cursor.get_token().get_locator() <=
                          cursor.get_token(1).get_locator()) {
                        return Binding::Failure::Rejected;
                      }

                      ttx_publication output = {};
                      auto position = cursor.get_abi();
                      const auto status = interpret(
                          Abstract::provide(language).get_query(),
                          &position, Abstract::provide(types).get_abi(),
                          &output);
                      cursor.set_index(position.index);
                      inspect(cursor, status);
                      if (status != TTX_BINDING_SATISFIED) {
                        return Binding::Failure::Rejected;
                      }

                      Publication graph(output);
                      source.close();
                      if (releases != 1) {
                        return Binding::Failure::Rejected;
                      }

                      // Cursor-owned spelling has been freed. All graph names
                      // and routes must have been copied before any later
                      // inspection.
                      return graph.get_query().bind<Abstract>().visit(
                          [&](Abstract root) {
                            return Terminal::Llvm::Execution::compile(root);
                          },
                          [](Binding::Failure failure)
                              -> Utility::Result<
                                  Terminal::Llvm::Execution, Binding::Failure> {
                            return failure;
                          });
                    },
                    [](Binding::Failure failure)
                        -> Utility::Result<
                            Terminal::Llvm::Execution, Binding::Failure> {
                      return failure;
                    });
              },
              [](Core::View::Bytes error)
                  -> Utility::Result<
                      Terminal::Llvm::Execution, Binding::Failure> {
                Core::Diagnostics::Log::fatal(error);
              });
}

PERIMORTEM_UNIT_TEST(DialectTests, foreign_cursor_terminal) {
  // Compilation returns after the foreign Cursor, its loaded module, the
  // model graph and Type scope have all ended. Execution uses only the emitted
  // terminal. Long names also exercise the former U8 token-length failure.
  Memory::Dynamic::Bytes text("public "_view);
  text.append('f', 300);
  text.concat(" : func = [.value : U32] -> [U32] { return value; }"_view);
  compile_foreign(
      text,
      [&](Source::Lexical::Cursor, ttx_binding_status status) {
        EXPECT(status == TTX_BINDING_SATISFIED);
      })
      .visit(
          [&](Terminal::Llvm::Execution& artifact) {
            Validation::ModelTests::Image image(artifact);
            ASSERT(image.is_set());
            Ttx::Semantic::Realization::Invocation invoke;
            ASSERT(
                invoke.connect(
                    image.get_query(), operation, artifact.get_inputs(),
                    artifact.get_outputs()) == Binding::Status::Satisfied);
            U32 input = 391;
            U32 output = 0;
            EXPECT(
                invoke.invoke(&input, &output) == Ttx::Data::Status::Success);
            EXPECT_EQ(output, input);
          },
          [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(DialectTests, native_publication) {
  Memory::Allocator::Arena arena;
  const auto text = "public answer : func = [] -> [U32] { return 42; }"_view;
  const Source::Contents::Memory input(text, representation(text, arena));
  const auto origin = Abstract::provide(input);
  DialectTypes types;

  const auto tokenizer =
      required(Abstract::provide(lexer).bind<Source::Tokenization>());
  auto source = required(tokenizer.tokenize(origin));
  auto cursor = required(source.get_query().bind<Source::Lexical::Cursor>());
  const auto dialect =
      required(Abstract::provide(language).bind<Source::Dialect>());
  auto graph = required(dialect.interpret(cursor, Abstract::provide(types)));
  source.close();

  const auto root = required(graph.get_query().bind<Abstract>());
  EXPECT(root.get_data() == "answer"_view);
  const auto declaration = required(root.bind<Source::Declaration>());
  EXPECT(declaration.get_anchor()->get_source() == origin);
  Terminal::Llvm::Execution::compile(root).visit(
      [&](Terminal::Llvm::Execution& artifact) {
        // Terminal emission ends graph borrowing. Invocation below therefore
        // needs neither the Cursor nor the graph that described this function.
        graph.close();
        Validation::ModelTests::Image image(artifact);
        ASSERT(image.is_set());
        Ttx::Semantic::Realization::Invocation invoke;
        ASSERT(
            invoke.connect(
                image.get_query(), operation, artifact.get_inputs(),
                artifact.get_outputs()) == Binding::Status::Satisfied);
        U32 value = 0;
        EXPECT(invoke.invoke(nullptr, &value) == Ttx::Data::Status::Success);
        EXPECT_EQ(value, U32(42));
      },
      [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(DialectTests, forked_dialect) {
  Memory::Allocator::Arena arena;
  const auto text =
      "public first : func = [] -> [] {} public second : func = [] -> [] {}"_view;
  const Source::Contents::Memory input(text, representation(text, arena));
  DialectTypes types;
  const auto tokenizer =
      required(Abstract::provide(lexer).bind<Source::Tokenization>());
  auto source = required(tokenizer.tokenize(Abstract::provide(input)));
  auto cursor = required(source.get_query().bind<Source::Lexical::Cursor>());
  const auto dialect =
      required(Abstract::provide(language).bind<Source::Dialect>());

  // A speculative interpretation advances the fork and leaves the caller's
  // cached observation intact. Adopting the index is a separate caller choice.
  EXPECT(cursor.get_text() == "public"_view);
  auto branch = cursor;
  auto first = required(dialect.interpret(branch, Abstract::provide(types)));
  EXPECT(
      required(first.get_query().bind<Abstract>()).get_data() == "first"_view);
  EXPECT_EQ(cursor.get_index(), U64(0));
  EXPECT(branch.get_index() > cursor.get_index());
  EXPECT(cursor.get_text() == "public"_view);

  cursor.set_index(branch.get_index());
  auto second = required(dialect.interpret(cursor, Abstract::provide(types)));
  EXPECT(
      required(second.get_query().bind<Abstract>()).get_data() == "second"_view);
  EXPECT(cursor.matches(Source::Lexical::Code::Type::Terminal));
  EXPECT_EQ(cursor.get_error_count(), Count(0));
}

PERIMORTEM_UNIT_TEST(DialectTests, rejected_publication) {
  Memory::Allocator::Arena arena;
  const auto text = "public f : func = [.a : U32, .a : U32] -> [] {}"_view;
  const Source::Contents::Memory input(text, representation(text, arena));
  DialectTypes types;
  const auto tokenizer =
      required(Abstract::provide(lexer).bind<Source::Tokenization>());
  auto source = required(tokenizer.tokenize(Abstract::provide(input)));
  auto cursor = required(source.get_query().bind<Source::Lexical::Cursor>());
  const auto dialect =
      required(Abstract::provide(language).bind<Source::Dialect>());

  EXPECT(cursor.get_text() == "public"_view);
  dialect.interpret(cursor, Abstract::provide(types))
      .visit(
          [&](Publication&) { EXPECT(False); },
          [&](Binding::Failure failure) {
            EXPECT(failure == Binding::Failure::Rejected);
          });
  // The failed graph is destroyed, while copied errors remain accessible
  // through the caller's independently owned Cursor publication.
  ASSERT_EQ(cursor.get_error_count(), Count(1));
  EXPECT(cursor.get_index() > 0);
  EXPECT(cursor.get_text() == "U32"_view);
  EXPECT(
      cursor.get_error(0)->get_message() == "Duplicate parameter name."_view);
}

PERIMORTEM_UNIT_TEST(DialectTests, foreign_cursor_error) {
  const auto text = "public f : func = [.a : U32, .a : U32] -> [] {}"_view;
  compile_foreign(
      text,
      [&](Source::Lexical::Cursor cursor, ttx_binding_status status) {
        EXPECT(status == TTX_BINDING_REJECTED);
        EXPECT(cursor.get_index() > 0);
        EXPECT(cursor.get_text() == "U32"_view);
        ASSERT_EQ(cursor.get_error_count(), Count(1));
        const auto error = cursor.get_error(0);
        ASSERT(error);
        EXPECT(error->get_message() == "Duplicate parameter name."_view);
        ASSERT(error->get_anchor());
        EXPECT_EQ(error->get_anchor()->get_focus()->get_size(), U64(1));
      })
      .visit(
          [&](Terminal::Llvm::Execution&) { EXPECT(False); },
          [&](Binding::Failure failure) {
            EXPECT(failure == Binding::Failure::Rejected);
          });
}
