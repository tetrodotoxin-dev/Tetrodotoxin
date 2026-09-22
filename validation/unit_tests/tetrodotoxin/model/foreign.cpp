// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/tetrodotoxin/model/fixtures/foreign.h"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/model/image.hpp"

#include <dlfcn.h>

#include "perimortem/core/time.hpp"

#include "tetrodotoxin/dialect/library/function.hpp"
#include "tetrodotoxin/model/execution/constant.hpp"
#include "tetrodotoxin/model/execution/field.hpp"
#include "tetrodotoxin/model/execution/function.hpp"
#include "tetrodotoxin/model/execution/parameter.hpp"
#include "tetrodotoxin/model/execution/return.hpp"
#include "tetrodotoxin/model/execution/value.hpp"
#include "tetrodotoxin/model/type/policies/conversion.hpp"
#include "tetrodotoxin/model/type/policies/unsigned.hpp"
#include "tetrodotoxin/model/type/storage.hpp"
#include "tetrodotoxin/source/contents/memory.hpp"
#include "tetrodotoxin/source/lexical/cursors/stream.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "ttx/concept/domain.hpp"
#include "ttx/semantic/transport/block.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;
using namespace Tetrodotoxin;

static Validation::Harness ForeignModels = {.name = "Model::Foreign"_view};

static auto forms() -> model_forms {
  return {
    &Binding::representation<Abstract>(),
    &Ttx::Data::Form::Compiled<
        Ttx::Data::Form::Native<U32>::reference>::get_representation(),
    &Binding::representation<Model::Type::Storage>(),
    &Binding::representation<Model::Execution::Field>(),
    &Binding::representation<Model::Execution::Parameter>(),
    &Binding::representation<Model::Execution::Return>(),
    &Binding::representation<Model::Execution::Function>(),
    &Binding::representation<Ttx::Semantic::Transport::Block::Access>(),
    &Binding::representation<Ttx::Concept::Domain>(),
    &Binding::representation<Model::Type::Policies::Conversion>(),
    &Binding::representation<Model::Execution::Value>(),
  };
}

static auto compiled(U8 mode)
    -> Utility::Result<Terminal::Llvm::Execution, Binding::Failure> {
  void* library = dlopen(
      ".bin/bin/validation/unit_tests/tetrodotoxin/model/libmodel_foreign.so",
      RTLD_NOW | RTLD_LOCAL);
  if (!library) {
    return Binding::Failure::Rejected;
  }

  auto open = reinterpret_cast<decltype(&model_fixture_open)>(
      dlsym(library, "model_fixture_open"));
  if (!open) {
    dlclose(library);
    return Binding::Failure::Rejected;
  }

  model_fixture fixture;
  const Abstract root(open(&fixture, forms(), mode));
  auto result = Terminal::Llvm::Execution::compile(root);
  fixture.alive = 0;
  // A stale graph thunk would either abort through the retired fixture or jump
  // into unloaded code. The resulting object must contain all required facts.
  dlclose(library);
  return result;
}

PERIMORTEM_UNIT_TEST(ForeignModels, released_provider) {
  for (U8 mode = 0; mode < 3; ++mode) {
    compiled(mode).visit(
        [&](Terminal::Llvm::Execution& artifact) {
          Validation::ModelTests::Image image(artifact);
          ASSERT(image.is_set());
          Ttx::Semantic::Realization::Invocation invocation;
          ASSERT(
              invocation.connect(
                  image.get_query(), artifact.get_operation(),
                  artifact.get_inputs(),
                  artifact.get_outputs()) == Binding::Status::Satisfied);
          U32 input = 789;
          U32 output = 0;
          EXPECT(
              invocation.invoke(
                  mode == 0 ? &input : nullptr,
                  mode == 2 ? nullptr : &output) == Ttx::Data::Status::Success);
          if (mode != 2) {
            EXPECT_EQ(output, mode == 0 ? input : U32(12345));
          }
        },
        [&](Binding::Failure) { EXPECT(False); });
  }
}

PERIMORTEM_UNIT_TEST(ForeignModels, policy_and_shape) {
  void* library = dlopen(
      ".bin/bin/validation/unit_tests/tetrodotoxin/model/libmodel_foreign.so",
      RTLD_NOW | RTLD_LOCAL);
  ASSERT(library);
  auto open = reinterpret_cast<decltype(&model_fixture_open)>(
      dlsym(library, "model_fixture_open"));
  ASSERT(open);
  model_fixture fixture;
  const Abstract root(open(&fixture, forms(), 0));
  const Binding::Failure failures[] = {
    Binding::Failure::Unsupported, Binding::Failure::Pending,
    Binding::Failure::Rejected};
  for (const auto failure : failures) {
    fixture.refusal = static_cast<ttx_binding_status>(failure);
    Terminal::Llvm::Execution::compile(root).visit(
        [&](auto&) { EXPECT(False); },
        [&](Binding::Failure error) { EXPECT(error == failure); });
  }

  fixture.refusal = TTX_BINDING_SATISFIED;
  fixture.wrong_parameter = 1;
  Terminal::Llvm::Execution::compile(root).visit(
      [&](auto&) { EXPECT(False); },
      [&](Binding::Failure error) {
        EXPECT(error == Binding::Failure::Rejected);
      });
  fixture.wrong_parameter = 0;
  // U16 is now a supported Type. Reject a constant whose Type advertises U16
  // while its actual Block publication still supplies U32, before reading it.
  fixture.mode = 1;
  fixture.forms.storage = &Ttx::Data::Form::Compiled<
      Ttx::Data::Form::Native<U16>::reference>::get_representation();
  Terminal::Llvm::Execution::compile(root).visit(
      [&](auto&) { EXPECT(False); },
      [&](Binding::Failure error) {
        EXPECT(error == Binding::Failure::Rejected);
      });
  dlclose(library);
}

PERIMORTEM_UNIT_TEST(ForeignModels, constant_block_read) {
  void* library = dlopen(
      ".bin/bin/validation/unit_tests/tetrodotoxin/model/libmodel_foreign.so",
      RTLD_NOW | RTLD_LOCAL);
  ASSERT(library);
  auto open = reinterpret_cast<decltype(&model_fixture_open)>(
      dlsym(library, "model_fixture_open"));
  ASSERT(open);
  model_fixture fixture;
  const Abstract root(open(&fixture, forms(), 1));
  Terminal::Llvm::Execution::compile(root).visit(
      [&](auto& artifact) {
        EXPECT_EQ(fixture.reads, Count(1));
        const auto observations = fixture.observations;
        const auto bindings = fixture.bindings;
        fixture.alive = 0;
        Validation::ModelTests::Image image(artifact);
        ASSERT(image.is_set());
        Ttx::Semantic::Realization::Invocation invocation;
        ASSERT(
            invocation.connect(
                image.get_query(), artifact.get_operation(),
                artifact.get_inputs(),
                artifact.get_outputs()) == Binding::Status::Satisfied);
        U32 observed = 0;
        const auto started = Core::Time::clock();
        for (Count i = 0; i < 1000000; ++i) {
          invocation.invoke(nullptr, &observed);
        }

        const auto elapsed =
            started.measure(Core::Time::clock()).convert_to_nanoseconds();
        Core::Static::Bytes<192> timing_buffer;
        Core::Writer::Textual timing(timing_buffer);
        timing << "One million retained calls: "_view << elapsed << " ns"_view;
        Validation::Test::log_message("Model::Foreign"_view, 0, timing);
        EXPECT_EQ(image.get_bindings(), Count(1));
        EXPECT_EQ(observed, U32(12345));
        EXPECT_EQ(fixture.observations, observations);
        EXPECT_EQ(fixture.bindings, bindings);
      },
      [&](Binding::Failure) { EXPECT(False); });
  dlclose(library);
}

struct ImportedTypes {
  Abstract type;
  auto get_data() const -> Core::View::Bytes { return {}; }
  auto resolve_concept(Core::View::Bytes) const -> Abstract { return type; }
};

PERIMORTEM_UNIT_TEST(ForeignModels, source_foreign_type) {
  void* library = dlopen(
      ".bin/bin/validation/unit_tests/tetrodotoxin/model/libmodel_foreign.so",
      RTLD_NOW | RTLD_LOCAL);
  ASSERT(library);
  auto open = reinterpret_cast<decltype(&model_fixture_open)>(
      dlsym(library, "model_fixture_open"));
  ASSERT(open);
  model_fixture fixture;
  const Abstract root(open(&fixture, forms(), 0));
  root.bind<Model::Execution::Function>().visit(
      [&](Model::Execution::Function declaration) {
        declaration.get_parameters()
            .get_subject(0)
            .bind<Model::Execution::Field>()
            .visit(
                [&](Model::Execution::Field field) {
                  // Library consumes a Type supplied by another system. It has
                  // no native Type to cast and no Library child to obtain from
                  // it.
                  ImportedTypes types{field.get_type()};
                  Memory::Allocator::Arena arena;
                  const auto text =
                      "public f : func = [.x : U32] -> [U32] { return x; }"_view;
                  const auto schema = Ttx::Data::Form::Schema::range(
                      Ttx::Data::Form::Native<U8>::reference, text.get_size(),
                      1, text.get_size(), 1);
                  const Ttx::Data::Form::Representation* form = nullptr;
                  Ttx::Data::Form::Representation::compile(schema, arena)
                      .visit(
                          [&](const auto& value) { form = &value; },
                          [&](Ttx::Data::Status) { EXPECT(False); });
                  ASSERT(form);
                  const auto& source =
                      arena.construct<Source::Contents::Memory>(text, *form);
                  Source::Lexical::Tokenizer tokenizer(
                      arena, text, "foreign-type.ttx"_view,
                      Abstract::provide(source));
                  Source::Errors errors;
                  Source::Lexical::Cursors::Stream traversal(tokenizer, errors);
                  auto cursor = traversal.get_interface();
                  const auto function = Dialect::Library::Function::interpret(
                      arena, cursor, Abstract::provide(types),
                      System::Uuid(1, 2));
                  ASSERT(function);
                  Terminal::Llvm::Execution::compile(
                      Abstract::provide(*function))
                      .visit(
                          [&](auto& artifact) {
                            EXPECT(!artifact.get_object().is_empty());
                          },
                          [&](Binding::Failure) { EXPECT(False); });
                },
                [&](Binding::Failure) { EXPECT(False); });
      },
      [&](Binding::Failure) { EXPECT(False); });
  dlclose(library);
}
