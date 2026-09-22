// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/llvm/execution.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"
#include "tetrodotoxin/model/execution/constant.hpp"
#include "tetrodotoxin/model/execution/field.hpp"
#include "tetrodotoxin/model/execution/function.hpp"
#include "tetrodotoxin/model/execution/parameter.hpp"
#include "tetrodotoxin/model/execution/return.hpp"
#include "tetrodotoxin/model/execution/value.hpp"
#include "tetrodotoxin/model/type/policies/conversion.hpp"
#include "tetrodotoxin/model/type/storage.hpp"
#include "ttx/semantic/flows/copy.hpp"
#include "ttx/semantic/transport/flow.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;
using namespace Tetrodotoxin::Model;
using namespace Tetrodotoxin::Terminal;

// The terminal snapshots declaration occurrences once. These keys associate a
// parameter use with an input slot, not a native Type or semantic equivalence.
// Both pointers belong to the admitted Abstract record and remain borrowed only
// during compilation.
struct Occurrence {
  const void* source;
  const ttx_abstract_ops* operations;
  explicit Occurrence(Abstract subject)
      : source(subject.get_abi().source),
        operations(subject.get_abi().operations) {}
  auto operator==(const Occurrence&) const -> bool = default;
  auto hash() const -> U64 {
    return Core::Hash(source).Rehash(Core::Hash(operations).get_value());
  }
};

struct Slot {
  Abstract declaration;
  Abstract type;
  const Ttx::Data::Form::Representation* form;
  Count offset;
};

using Slots = Memory::Dynamic::Vector<Slot>;
using Positions = Memory::Dynamic::Map<Occurrence, Count>;

static auto describe_frame(
    Execution::Layout layout,
    Memory::Allocator::Arena& arena,
    Slots& slots,
    const Ttx::Data::Form::Representation*& form)
    -> Core::Option<Binding::Failure> {
  using Ttx::Data::Form::Representation;
  Memory::Dynamic::Vector<Representation::Member> members;
  Count extent = 0;
  Count alignment = 1;
  const Count count = layout.get_size();
  for (Count index = 0; index < count; ++index) {
    const auto subject = layout.get_subject(index);
    const auto failure = subject.bind<Execution::Field>().visit(
        [&](Execution::Field field) -> Core::Option<Binding::Failure> {
          const auto type = field.get_type();
          return type.bind<Type::Storage>().visit(
              [&](Type::Storage storage) -> Core::Option<Binding::Failure> {
                return storage.get_representation().visit(
                    [&](const Representation& member)
                        -> Core::Option<Binding::Failure> {
                      const Count mask = member.get_alignment() - 1;
                      if (extent > Count(-1) - mask) {
                        return Binding::Failure::Rejected;
                      }

                      const Count offset = (extent + mask) & ~mask;
                      if (member.get_extent() > Count(-1) - offset) {
                        return Binding::Failure::Rejected;
                      }

                      slots.insert(Slot(subject, type, &member, offset));
                      members.insert(Representation::Member(member, offset));
                      extent = offset + member.get_extent();
                      alignment =
                          Core::Math::max(alignment, member.get_alignment());
                      return {};
                    },
                    [](Binding::Failure failure)
                        -> Core::Option<Binding::Failure> { return failure; });
              },
              [](Binding::Failure failure) -> Core::Option<Binding::Failure> {
                return failure;
              });
        },
        [](Binding::Failure failure) -> Core::Option<Binding::Failure> {
          return failure;
        });
    if (failure) {
      return failure;
    }
  }

  if (extent > Count(-1) - (alignment - 1)) {
    return Binding::Failure::Rejected;
  }

  extent = (extent + alignment - 1) & ~(alignment - 1);
  return Representation::compose(members.get_view(), extent, alignment, arena)
      .visit(
          [&](const Representation& result) -> Core::Option<Binding::Failure> {
            form = &result;
            return {};
          },
          [](Ttx::Data::Status) -> Core::Option<Binding::Failure> {
            return Binding::Failure::Rejected;
          });
}

static auto address(
    LLVMBuilderRef builder,
    LLVMContextRef context,
    LLVMValueRef base,
    Count offset) -> LLVMValueRef {
  auto index = LLVMConstInt(LLVMInt64TypeInContext(context), offset, false);
  return LLVMBuildGEP2(
      builder, LLVMInt8TypeInContext(context), base, &index, 1, "");
}

static auto lower_value(
    Abstract value,
    const Slot& output,
    const Slots& parameters,
    const Positions& positions,
    LLVMContextRef context,
    LLVMModuleRef module,
    LLVMBuilderRef builder,
    LLVMValueRef inputs) -> Utility::Result<LLVMValueRef, Binding::Failure> {
  using Result = Utility::Result<LLVMValueRef, Binding::Failure>;
  return value.bind<Execution::Parameter>().visit(
      [&](Execution::Parameter use) -> Result {
        return positions.find(Occurrence(use.get_field()))
            .visit(
                []() -> Result { return Binding::Failure::Rejected; },
                [&](const auto& found) -> Result {
                  if (found.value == Count(-1)) {
                    return Binding::Failure::Rejected;
                  }

                  const auto& source =
                      parameters.get_view().get_data()[found.value];
                  if (!source.form->compatible(*output.form)) {
                    return Binding::Failure::Rejected;
                  }

                  return address(builder, context, inputs, source.offset);
                });
      },
      [&](Binding::Failure failure) -> Result {
        if (failure != Binding::Failure::Unsupported) {
          return failure;
        }

        const auto immutable = value.supports<Execution::Constant>();
        if (immutable != Binding::Status::Satisfied) {
          return static_cast<Binding::Failure>(immutable);
        }

        return value.bind<Execution::Value>().visit(
            [&](Execution::Value constant) -> Result {
              // Providers choose how bytes become observable. The compiler
              // materializes one agreed observation before the graph dies,
              // then emits those bytes into the independently owned object.
              Ttx::Semantic::Transport::Flow flow;
              const auto status = flow.connect(
                  decltype(flow)::reader(*output.form), constant.get_value());
              if (status == decltype(flow)::Status::BindingPending) {
                return Binding::Failure::Pending;
              }

              if (status != decltype(flow)::Status::Success ||
                  output.form->get_extent() > U32(-1)) {
                return Binding::Failure::Rejected;
              }

              Memory::Dynamic::Bytes bytes;
              bytes.resize(output.form->get_extent());
              bytes.set(0);
              const Ttx::Data::Form::Storage storage(ttx_storage(
                  output.form, bytes.get_access().get_data(),
                  bytes.get_size()));
              if (Ttx::Semantic::Flows::Copy::flow(flow, storage) !=
                  Ttx::Data::Status::Success) {
                return Binding::Failure::Rejected;
              }

              const auto literal = LLVMConstStringInContext(
                  context,
                  reinterpret_cast<const char*>(bytes.get_view().get_data()),
                  unsigned(bytes.get_size()), true);
              const auto global =
                  LLVMAddGlobal(module, LLVMTypeOf(literal), "literal");
              LLVMSetInitializer(global, literal);
              LLVMSetGlobalConstant(global, true);
              LLVMSetLinkage(global, LLVMPrivateLinkage);
              return global;
            },
            [](Binding::Failure failure) -> Result { return failure; });
      });
}

static auto emit(
    Abstract body_subject,
    const Slots& parameters,
    const Slots& results,
    LLVMContextRef context,
    LLVMModuleRef module) -> Core::Option<Binding::Failure> {
  Positions positions;
  for (Count i = 0; i < parameters.get_size(); ++i) {
    const Occurrence key(parameters.get_view().get_data()[i].declaration);
    positions.find(key).visit(
        [&] { positions.insert(key, i); },
        [](auto& found) { found.value = Count(-1); });
  }

  return body_subject.bind<Execution::Return>().visit(
      [&](Execution::Return body) -> Core::Option<Binding::Failure> {
        const auto values = body.get_values();
        if (values.get_size() != results.get_size()) {
          return Binding::Failure::Rejected;
        }

        LLVMTypeRef args[] = {
          LLVMPointerTypeInContext(context, 0),
          LLVMPointerTypeInContext(context, 0)};
        const auto signature =
            LLVMFunctionType(LLVMVoidTypeInContext(context), args, 2, false);
        const auto entry = LLVMAddFunction(module, "ttx_entry", signature);
        const auto builder = LLVMCreateBuilderInContext(context);
        LLVMPositionBuilderAtEnd(
            builder, LLVMAppendBasicBlockInContext(context, entry, "entry"));
        Core::Option<Binding::Failure> failed;
        for (Count i = 0; i < results.get_size(); ++i) {
          using Result = Utility::Result<LLVMValueRef, Binding::Failure>;
          const auto& slot = results.get_view().get_data()[i];
          slot.type.bind<Type::Policies::Conversion>()
              .visit(
                  [&](Type::Policies::Conversion conversion) -> Result {
                    return conversion.convert(values.get_subject(i))
                        .visit(
                            [&](Abstract projected) -> Result {
                              return lower_value(
                                  projected, slot, parameters, positions,
                                  context, module, builder,
                                  LLVMGetParam(entry, 0));
                            },
                            [](Binding::Failure failure) -> Result {
                              return failure;
                            });
                  },
                  [](Binding::Failure failure) -> Result { return failure; })
              .visit(
                  [&](LLVMValueRef value) {
                    LLVMBuildMemMove(
                        builder,
                        address(
                            builder, context, LLVMGetParam(entry, 1),
                            slot.offset),
                        1, value, 1,
                        LLVMConstInt(
                            LLVMInt64TypeInContext(context),
                            slot.form->get_extent(), false));
                  },
                  [&](Binding::Failure failure) { failed = failure; });
          if (failed) {
            break;
          }
        }

        if (!failed) {
          LLVMBuildRetVoid(builder);
        }

        LLVMDisposeBuilder(builder);
        return failed;
      },
      [](Binding::Failure failure) -> Core::Option<Binding::Failure> {
        return failure;
      });
}

// Object emission uses the same target backend as the existing LLVM terminal.
// Linking and executable code lifetime belong to its consumer, so compilation
// need not introduce a runtime engine or retain LLVM construction state.
static auto object_code(LLVMModuleRef module, Memory::Dynamic::Bytes& output)
    -> Bool {
  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86Target();
  LLVMInitializeX86TargetMC();
  LLVMInitializeX86AsmPrinter();
  const char* triple = "x86_64-unknown-linux-gnu";
  char* message = nullptr;
  LLVMTargetRef target = nullptr;
  if (LLVMGetTargetFromTriple(triple, &target, &message)) {
    LLVMDisposeMessage(message);
    return False;
  }

  const auto machine = LLVMCreateTargetMachine(
      target, triple, "generic", "", LLVMCodeGenLevelDefault, LLVMRelocPIC,
      LLVMCodeModelDefault);
  const auto layout = LLVMCreateTargetDataLayout(machine);
  LLVMSetTarget(module, triple);
  LLVMSetModuleDataLayout(module, layout);
  LLVMMemoryBufferRef buffer = nullptr;
  const Bool failed =
      LLVMTargetMachineEmitToMemoryBuffer(
          machine, module, LLVMObjectFile, &message, &buffer) != 0;
  LLVMDisposeTargetData(layout);
  LLVMDisposeTargetMachine(machine);
  if (failed) {
    LLVMDisposeMessage(message);
    return False;
  }

  output = Memory::Dynamic::Bytes(
      Core::View::Bytes(
          reinterpret_cast<const U8*>(LLVMGetBufferStart(buffer)),
          LLVMGetBufferSize(buffer)));
  LLVMDisposeMemoryBuffer(buffer);
  return True;
}

auto Llvm::Execution::compile(Abstract subject)
    -> Utility::Result<Execution, Binding::Failure> {
  const auto context = LLVMContextCreate();
  const auto module = LLVMModuleCreateWithNameInContext("execution", context);
  auto result = subject.bind<Model::Execution::Function>().visit(
      [&](Model::Execution::Function function)
          -> Utility::Result<Execution, Binding::Failure> {
        Execution output;
        const auto input_layout = function.get_parameters();
        const auto output_layout = function.get_results();
        Slots parameters;
        Slots results;
        if (auto error = describe_frame(
                input_layout, output.arena, parameters, output.inputs)) {
          return *error;
        }

        if (auto error = describe_frame(
                output_layout, output.arena, results, output.outputs)) {
          return *error;
        }

        if (auto error = emit(
                function.get_body(), parameters, results, context, module)) {
          return *error;
        }

        // The executable owns no graph edges. Metadata needed by Invocation is
        // copied into its Arena before the caller may release the publication.
        output.operation = function.get_operation();
        char* message = nullptr;
        if (LLVMVerifyModule(module, LLVMReturnStatusAction, &message)) {
          Core::Diagnostics::Log::error(
              "Execution module failed LLVM verification."_view);
          LLVMDisposeMessage(message);
          return Binding::Failure::Rejected;
        }

        LLVMDisposeMessage(message);
        if (!object_code(module, output.object)) {
          return Binding::Failure::Rejected;
        }

        return static_cast<Execution&&>(output);
      },
      [](Binding::Failure error)
          -> Utility::Result<Execution, Binding::Failure> { return error; });
  LLVMDisposeModule(module);
  LLVMContextDispose(context);
  return result;
}
