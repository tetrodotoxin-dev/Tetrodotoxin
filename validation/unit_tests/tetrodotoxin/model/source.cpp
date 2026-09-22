// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/model/image.hpp"

#include "tetrodotoxin/dialect/library/function.hpp"
#include "tetrodotoxin/dialect/library/type_reference.hpp"
#include "tetrodotoxin/model/type/primitives/u32.hpp"
#include "tetrodotoxin/source/contents/memory.hpp"
#include "tetrodotoxin/source/lexical/cursors/stream.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "ttx/concept/answers/none.hpp"
#include "ttx/concept/answers/unknown.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;
using namespace Tetrodotoxin;

static Validation::Harness SourceModels = {
  .name = "Dialect::Library::Model"_view};
static constexpr System::Uuid operation(0xcbe08a8bcd994e9a, 0x968e8c88f6a24499);

struct Types {
  Model::Type::Primitives::U32 type;
  bool ready = true;
  auto get_data() const -> Core::View::Bytes { return {}; }
  auto resolve_concept(Core::View::Bytes route) const -> Abstract {
    if (!ready) {
      return Answers::Unknown::get_unknown();
    }

    return route == "U32"_view ? Abstract::provide(type)
                               : Answers::None::get_none();
  }
};

static auto observation(Memory::Allocator::Arena& arena, Core::View::Bytes text)
    -> const Source::Contents::Memory& {
  using namespace Ttx::Data::Form;
  const auto schema = Schema::range(
      Native<U8>::reference, text.get_size(), 1, text.get_size(), 1);
  const auto& form =
      Representation::compile(schema, arena)
          .visit(
              [](const Representation& form) -> const Representation& {
                return form;
              },
              [](Ttx::Data::Status) -> const Representation& {
                Core::Diagnostics::Log::fatal(
                    "Invalid source fixture representation."_view);
              });
  return arena.construct<Source::Contents::Memory>(text, form);
}

static auto compile(Core::View::Bytes text)
    -> Utility::Result<Terminal::Llvm::Execution, Binding::Failure> {
  Memory::Allocator::Arena arena;
  Types types;
  const auto& source = observation(arena, text);
  Core::Option<Dialect::Library::Function&> function;
  {
    // Lexical storage ends before model compilation. The parser owns graph
    // allocation explicitly and copies token coordinates into its source facts.
    // Text still belongs to the caller's observation through this whole call.
    Memory::Allocator::Arena lexical;
    Source::Lexical::Tokenizer tokenizer(
        lexical, text, "fixture.ttx"_view, Abstract::provide(source));
    Source::Errors errors;

    Source::Lexical::Cursors::Stream traversal(tokenizer, errors);
    auto cursor = traversal.get_interface();
    function = Dialect::Library::Function::interpret(
        arena, cursor, Abstract::provide(types), operation);
    if (!function || !errors.is_empty() ||
        !cursor.matches(Source::Lexical::Code::Type::Terminal)) {
      return Binding::Failure::Rejected;
    }
  }

  return Terminal::Llvm::Execution::compile(Abstract::provide(*function));
}

PERIMORTEM_UNIT_TEST(SourceModels, source_independence) {
  // Source text and interpretation objects have disappeared before loading the
  // generated object. The code retains no parser or source Type provider.
  compile(
      "public identity : func = [.value : U32] -> [U32] { return value; }"_view)
      .visit(
          [&](Terminal::Llvm::Execution& artifact) {
            Validation::ModelTests::Image image(artifact);
            ASSERT(image.is_set());
            Ttx::Semantic::Realization::Invocation invoke;
            ASSERT(
                invoke.connect(
                    image.get_query(), operation, artifact.get_inputs(),
                    artifact.get_outputs()) == Binding::Status::Satisfied);
            const U32 input = 103;
            U32 output = 0;
            EXPECT(
                invoke.invoke(&input, &output) == Ttx::Data::Status::Success);
            EXPECT_EQ(output, input);
          },
          [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(SourceModels, constant_and_entry) {
  compile("public answer : func = [] -> [U32] { return 42; }"_view)
      .visit(
          [&](Terminal::Llvm::Execution& artifact) {
            Validation::ModelTests::Image image(artifact);
            ASSERT(image.is_set());
            Ttx::Semantic::Realization::Invocation invoke;
            ASSERT(
                invoke.connect(
                    image.get_query(), operation, artifact.get_inputs(),
                    artifact.get_outputs()) == Binding::Status::Satisfied);
            U32 output = 0;
            EXPECT(
                invoke.invoke(nullptr, &output) == Ttx::Data::Status::Success);
            EXPECT_EQ(output, U32(42));
          },
          [&](Binding::Failure) { EXPECT(False); });

  // An App entry requirement is a signature question. Empty frames require
  // neither a Library identity nor fabricated storage for an implicit value.
  compile("public main : func = [] -> [] {}"_view)
      .visit(
          [&](Terminal::Llvm::Execution& artifact) {
            Validation::ModelTests::Image image(artifact);
            ASSERT(image.is_set());
            EXPECT_EQ(artifact.get_inputs().get_extent(), Count(0));
            EXPECT_EQ(artifact.get_outputs().get_extent(), Count(0));
            Ttx::Semantic::Realization::Invocation invoke;
            ASSERT(
                invoke.connect(
                    image.get_query(), operation, artifact.get_inputs(),
                    artifact.get_outputs()) == Binding::Status::Satisfied);
            EXPECT(
                invoke.invoke(nullptr, nullptr) == Ttx::Data::Status::Success);
          },
          [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(SourceModels, provenance_and_pending) {
  Memory::Allocator::Arena arena;
  const auto text =
      "public identity : func = [.value : U32] -> [U32] { return value; }"_view;
  const auto& source = observation(arena, text);
  Source::Lexical::Tokenizer tokenizer(
      arena, text, "source.ttx"_view, Abstract::provide(source));
  Source::Errors errors;

  Source::Lexical::Cursors::Stream traversal(tokenizer, errors);
  auto cursor = traversal.get_interface();
  Types types;
  types.ready = false;
  auto parsed = Dialect::Library::Function::interpret(
      arena, cursor, Abstract::provide(types), operation);
  ASSERT(parsed);
  ASSERT(errors.is_empty());
  const auto function = Abstract::provide(*parsed);
  function.bind<Source::Declaration>().visit(
      [&](Source::Declaration declaration) {
        ASSERT(declaration.get_anchor());
        EXPECT_EQ(declaration.get_anchor()->get_extent().get_offset(), U64(0));
      },
      [&](Binding::Failure) { EXPECT(False); });
  function.resolve_concept("value"_view)
      .bind<Source::Declaration>()
      .visit(
          [&](Source::Declaration declaration) {
            ASSERT(declaration.get_anchor());
            const auto anchor = *declaration.get_anchor();
            EXPECT(anchor.get_source() == Abstract::provide(source));
            const auto extent = anchor.get_extent();
            EXPECT(
                tokenizer.get_source_text().slice(
                    extent.get_offset(), extent.get_size()) ==
                ".value : U32"_view);
          },
          [&](Binding::Failure) { EXPECT(False); });
  Abstract::provide(types.type)
      .bind<Source::Declaration>()
      .visit(
          [&](Source::Declaration) { EXPECT(False); },
          [&](Binding::Failure error) {
            EXPECT(error == Binding::Failure::Unsupported);
          });

  Terminal::Llvm::Execution::compile(function).visit(
      [&](auto&) { EXPECT(False); },
      [&](Binding::Failure error) {
        EXPECT(error == Binding::Failure::Pending);
      });
  // Only the authority changes. No reparsing or completion driver mutates the
  // graph, and there is no native Type cache to repair before querying again.
  types.ready = true;
  Terminal::Llvm::Execution::compile(function).visit(
      [&](auto& artifact) { EXPECT(!artifact.get_object().is_empty()); },
      [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(SourceModels, rejected_source) {
  const Core::View::Bytes sources[] = {
    "public f : func = [.a : U32, .a : U32] -> [U32] { return a; }"_view,
    "public f : func = [] -> [U32] { return 4294967296; }"_view,
    "public f : func = [] -> [U32] { return absent; }"_view,
    "public f : func = [] -> [U32] {}"_view,
    "public f : func = [] -> [] { return 1; }"_view,
  };
  for (const auto source : sources) {
    compile(source).visit(
        [&](auto&) { EXPECT(False); },
        [&](Binding::Failure error) {
          EXPECT(error == Binding::Failure::Rejected);
        });
  }
}

struct TypePolicy {
  Model::Type::Primitives::U32 type;
  bool restricted = false;
  auto get_data() const -> Core::View::Bytes { return {}; }
  auto resolve() const -> Abstract { return Abstract::provide(type); }
  auto supports(System::Uuid id) const -> Binding::Status {
    return restricted ? Binding::Status::Rejected : type.supports(id);
  }
  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage output) const
      -> Binding::Status {
    return restricted ? Binding::Status::Rejected
                      : type.bind_interface(id, output);
  }

  auto resolve_concept(Core::View::Bytes) const -> Abstract {
    return Answers::None::get_none();
  }

  auto visit_concepts(Abstract::Visitor visitor) const -> void {
    visitor("visible"_view, Answers::None::get_none());
  }
};

struct PolicyNamespace {
  Abstract policy;
  auto get_data() const -> Core::View::Bytes { return {}; }
  auto resolve_concept(Core::View::Bytes) const -> Abstract { return policy; }
};

PERIMORTEM_UNIT_TEST(SourceModels, retained_type_policy) {
  TypePolicy policy;
  PolicyNamespace names{Abstract::provide(policy)};
  Memory::Allocator::Arena arena;
  const auto& source = observation(arena, "U32"_view);
  const Dialect::Library::TypeReference reference(
      Abstract::provide(names), "U32"_view,
      Source::Anchor(Abstract::provide(source), Source::Range(0, 3)));
  const auto subject = Abstract::provide(reference);
  Count visits = 0;
  auto receive = [&](Core::View::Bytes route, Abstract value) {
    ++visits;
    EXPECT(route == "visible"_view);
    EXPECT(subject.resolve_concept(route) == value);
  };
  subject.visit_concepts(Abstract::Visitor(receive));
  EXPECT_EQ(visits, Count(1));

  // The source reference must ask the encountered policy, although resolving
  // that policy would expose a permissive native type underneath it.
  policy.restricted = true;
  EXPECT(
      subject.supports<Model::Type::Policies::Unsigned>() ==
      Binding::Status::Rejected);
  EXPECT(subject.resolve() == subject);
}
