// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// The native bridge enters LLVM before the Perimortem owner so LLVM's standard
// declarations remain confined to this implementation unit.
#if __has_include("llvm/IR/Function.h")
#include "llvm/IR/Function.h"
#else
#error LLVM Function is required by the Library native compiler
#endif

#include "perimortem/core/diagnostics/log.hpp"

#include "llvm-c/Core.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CBindingWrapping.h"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/terminal/abi/export.hpp"
#include "tetrodotoxin/terminal/abi/publication.hpp"
#include "tetrodotoxin/terminal/abi/symbol.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/execution.hpp"
#include "tetrodotoxin/terminal/llvm/module/body.hpp"
#include "tetrodotoxin/terminal/llvm/module/c_abi.hpp"
#include "tetrodotoxin/terminal/llvm/module/carriers.hpp"
#include "tetrodotoxin/terminal/llvm/module/functions.hpp"
#include "tetrodotoxin/terminal/llvm/module/program.hpp"
#include "tetrodotoxin/source/addressable.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

static auto select_type(const Tetrodotoxin::Source::Abstract& answer)
    -> Core::Option<const Tetrodotoxin::Source::Type&> {
  auto direct = answer.select<Tetrodotoxin::Source::Type>();
  return direct ? direct : answer.resolve().select<Tetrodotoxin::Source::Type>();
}

static auto select_program(Llvm::Module::Emission& program)
    -> Core::Option<Llvm::Module::Program&> {
  if (program.get_kind() == Llvm::Module::Emission::Kind::Module) {
    return static_cast<Llvm::Module::Program&>(program);
  }
  return static_cast<Llvm::Module::Body&>(program).get_program();
}

static auto is_local_definition(
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Language::Definition& definition) -> Bool {
  Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract> current(
      definition.get_host());
  while (true) {
    auto source =
        current.get().select<Tetrodotoxin::Library::Language::Types::Source>();
    if (source) {
      return unit.owns(source->get_host());
    }
    auto composite =
        current.get()
            .select<Tetrodotoxin::Library::Language::Types::Composite>();
    BAIL_IF(!composite);
    current = composite->get_definition().get_host();
  }
}

static auto get_program(Llvm::Module::Emission& body)
    -> Llvm::Module::Program& {
  return body.get_kind() == Llvm::Module::Emission::Kind::Body
             ? static_cast<Llvm::Module::Body&>(body).get_program()
             : static_cast<Llvm::Module::Program&>(body);
}

static auto select_body(Llvm::Module::Emission& emission)
    -> Core::Option<Llvm::Module::Body&> {
  return emission.get_kind() == Llvm::Module::Emission::Kind::Body
             ? Core::Option<Llvm::Module::Body&>(
                   static_cast<Llvm::Module::Body&>(emission))
             : Core::Option<Llvm::Module::Body&>();
}

static auto select_carriers(Llvm::Module::Emission& program)
    -> Core::Option<const Llvm::Module::Carriers&> {
  auto native = select_program(program);
  return native ? Core::Option<const Llvm::Module::Carriers&>(
                      native->get_carriers())
                : Core::Option<const Llvm::Module::Carriers&>();
}

static auto get_context(Llvm::Module::Program& program) -> llvm::LLVMContext& {
  return *llvm::unwrap(&program.get_context());
}

static auto get_module(Llvm::Module::Program& program) -> llvm::Module& {
  return *llvm::unwrap(&program.get_module());
}

static auto get_builder(Llvm::Module::Body& body) -> llvm::IRBuilder<>& {
  return *llvm::unwrap(body.get_builder());
}

static auto fail_toolchain(
    Llvm::Module::Emission& program,
    Core::View::Bytes message) -> Bool {
  auto target = select_program(program);
  return target ? target->fail_toolchain(message) : False;
}

static auto fail_callable(
    Llvm::Module::Emission& program,
    Core::Option<const Tetrodotoxin::Language::Definition&> definition,
    Core::View::Bytes message,
    Core::View::Bytes hint = {}) -> Bool {
  auto target = select_program(program);
  auto anchor = definition.visit(
      []() { return Core::Option<Tetrodotoxin::Source::Lexical::Anchor>(); },
      [](const Tetrodotoxin::Language::Definition& selected) {
        return Core::Option<Tetrodotoxin::Source::Lexical::Anchor>(selected.get_anchor());
      });
  if (target && anchor) {
    return target->fail_source(*anchor, message, hint);
  }

  return fail_toolchain(program, message);
}

static auto declares_self(const Tetrodotoxin::Source::Callable& callable) -> Bool {
  auto first = callable.get_parameters().get_abstract(0);
  auto parameter = first ? first->select<Tetrodotoxin::Source::Addressable>()
                         : Core::Option<const Tetrodotoxin::Source::Addressable&>();
  return parameter && parameter->get_name() == "self"_view;
}

static auto uses_memory_abi(Llvm::Module::Program& program, llvm::Type& type)
    -> Bool {
  llvm::Module& module = get_module(program);
  return Bool(
      type.isAggregateType() &&
      module.getDataLayout().getTypeAllocSize(&type).getFixedValue() > 16);
}

static auto count_c_registers(llvm::Type& type, Count& integers, Count& sse)
    -> void {
  if (auto* structure = llvm::dyn_cast<llvm::StructType>(&type)) {
    for (llvm::Type* element : structure->elements()) {
      count_c_registers(*element, integers, sse);
    }
    return;
  }
  if (type.isFloatingPointTy() || type.isVectorTy()) {
    sse++;
  } else {
    integers++;
  }
}

static auto get_extension(
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Type& type) -> Core::Option<llvm::Attribute::AttrKind> {
  auto native = carriers.get_type(type);
  if (!native) {
    return {};
  }

  auto integer = llvm::dyn_cast<llvm::IntegerType>(llvm::unwrap(*native));
  if (!integer || integer->getBitWidth() >= 32 || carriers.is_real(type)) {
    return {};
  }

  return carriers.is_signed(type) ? llvm::Attribute::SExt
                                  : llvm::Attribute::ZExt;
}

static auto select_parameter_types(
    Llvm::Module::Emission& program,
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Callable& callable,
    Memory::Dynamic::Vector<llvm::Type*>& native,
    Memory::Dynamic::Vector<const Tetrodotoxin::Source::Type*>& semantic) -> Bool {
  const Tetrodotoxin::Source::Layout& layout = callable.get_parameters();
  for (Count index = 0; index < layout.get_size(); index++) {
    auto entry = layout.get_abstract(index);
    auto parameter = entry ? entry->select<Tetrodotoxin::Source::Addressable>()
                           : Core::Option<const Tetrodotoxin::Source::Addressable&>();
    if (!parameter) {
      return fail_toolchain(
          program,
          "LLVM received a Callable parameter without an Addressable."_view);
    }

    auto semantic_type = select_type(parameter->get_type());
    auto type = semantic_type ? carriers.get_type(*semantic_type)
                              : Core::Option<LLVMTypeRef>();
    if (!semantic_type || !type) {
      return fail_toolchain(
          program,
          "LLVM cannot find the completed carrier for a Callable parameter."_view);
    }

    native.insert(llvm::unwrap(*type));
    semantic.insert(&*semantic_type);
  }

  return True;
}

static auto select_result_types(
    Llvm::Module::Emission& program,
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Callable& callable,
    Memory::Dynamic::Vector<llvm::Type*>& native,
    Memory::Dynamic::Vector<const Tetrodotoxin::Source::Type*>& semantic) -> Bool {
  const Tetrodotoxin::Source::Layout& layout = callable.get_results();
  auto library_callable =
      callable.select<Tetrodotoxin::Library::Language::Model::Callable>();
  auto self_result = library_callable
                         ? library_callable->get_self_result()
                         : Core::Option<const Tetrodotoxin::Source::Addressable&>();
  auto target = select_program(program);
  for (Count index = 0; index < layout.get_size(); index++) {
    auto entry = layout.get_abstract(index);
    auto addressable = entry ? entry->select<Tetrodotoxin::Source::Addressable>()
                             : Core::Option<const Tetrodotoxin::Source::Addressable&>();
    auto type = addressable ? select_type(addressable->get_type())
                : entry     ? select_type(*entry)
                            : Core::Option<const Tetrodotoxin::Source::Type&>();
    if (!type || !target) {
      return fail_toolchain(
          program,
          "LLVM received a Callable result without an exact Type."_view);
    }

    llvm::Type* carrier =
        self_result && addressable && &*self_result == &*addressable
            ? llvm::PointerType::getUnqual(get_context(*target))
            : carriers.get_type(*type).visit(
                  []() -> llvm::Type* { return nullptr; },
                  [](LLVMTypeRef selected) -> llvm::Type* {
                    return llvm::unwrap(selected);
                  });
    if (!carrier) {
      return fail_toolchain(
          program,
          "LLVM cannot find the completed carrier for a Callable result."_view);
    }

    native.insert(carrier);
    semantic.insert(&*type);
  }

  return True;
}

auto Llvm::Module::Functions::reserve(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Callable& callable,
    Record record) const -> Core::Option<Bool> {
  auto found = records.find(&callable);
  if (found) {
    if (found->value.kind != record.kind) {
      fail_toolchain(
          program,
          "LLVM received different owners for one Callable identity."_view);
      return {};
    }

    return False;
  }

  records.insert(&callable, record);
  return True;
}

auto Llvm::Module::Functions::reserve_function(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Callable& callable,
    const Tetrodotoxin::Language::Definition& definition) const
    -> Core::Option<Bool> {
  auto target = select_program(program);
  if (!target || !target->get_unit().is_package_member() ||
      is_local_definition(target->get_unit(), definition)) {
    return reserve(
        program, callable, Record(Kind::Function, {}, {}, definition));
  }

  auto symbol = target->get_unit().find(callable);
  if (!symbol) {
    fail_callable(
        program, definition,
        "LLVM Package member is missing one external Callable binding."_view);
    return {};
  }
  return reserve(
      program, callable, Record(Kind::External, {}, *symbol, definition));
}

auto Llvm::Module::Functions::reserve_foreign(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Callable& callable,
    Core::View::Bytes abi,
    Core::View::Bytes symbol) const -> Core::Option<Bool> {
  if (abi != "C"_view) {
    fail_callable(
        program, {}, "LLVM supports only the authored Foreign C ABI."_view);
    return {};
  }

  if (!Tetrodotoxin::Terminal::Abi::Symbol::validate(symbol)) {
    fail_callable(
        program, {},
        "The Foreign Callable symbol is not a valid C identifier."_view);
    return {};
  }

  auto target = select_program(program);
  BAIL_IF(!target);

  auto reserved =
      reserve(program, callable, Record(Kind::Foreign, abi, symbol));
  if (reserved && *reserved) {
    foreign_callables.insert(callable);
    if (!target->add_import(
            Tetrodotoxin::Linker::Import(
                Tetrodotoxin::Linker::Import::Kind::Function, abi, symbol))) {
      return {};
    }
  }

  return reserved;
}

auto Llvm::Module::Functions::reserve_construction(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& owner,
    Bool provider,
    Core::View::Vector<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable>>
        parameters) const -> Bool {
  auto target = select_program(program);
  BAIL_IF(!target);

  auto found = constructions.find(&owner);
  if (found) {
    if (found->value.provider != provider ||
        found->value.parameters.get_size() != parameters.get_size()) {
      return fail_toolchain(
          program,
          "LLVM Type construction changed its reserved target facts."_view);
    }
    for (Count index = 0; index < parameters.get_size(); index++) {
      if (&found->value.parameters[index].get() !=
          &parameters.get_data()[index].get()) {
        return fail_toolchain(
            program,
            "LLVM Type construction changed its reserved target facts."_view);
      }
    }
    return True;
  }

  auto external = target->get_unit().find(owner);
  if (!provider && !external) {
    return fail_toolchain(
        program,
        "LLVM Package member is missing one external Type construction "
        "binding."_view);
  }

  auto publication = target->get_interface().find_publication(owner);
  Tetrodotoxin::Terminal::Abi::Symbol generated(
      target->get_arena(), owner,
      Tetrodotoxin::Terminal::Abi::Symbol::Kind::Construction,
      target->get_unit());
  Core::View::Bytes symbol = external      ? *external
                             : publication ? publication->get_symbol()
                                           : generated.get_view();
  ConstructionRecord record(provider, symbol);
  for (const Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable>& parameter :
       parameters) {
    record.parameters.insert(parameter);
  }
  constructions.insert(&owner, static_cast<ConstructionRecord&&>(record));
  return True;
}

auto Llvm::Module::Functions::complete_construction(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& owner) const -> Bool {
  auto target = select_program(program);
  auto carriers = select_carriers(program);
  auto found = constructions.find(&owner);
  if (!target || !carriers || !found) {
    return fail_toolchain(
        program,
        "LLVM cannot complete Type construction before reservation."_view);
  }

  ConstructionRecord& record = found->value;
  if (record.function) {
    return True;
  }

  auto result = carriers->get_type(owner);
  if (!result) {
    return fail_toolchain(
        program,
        "LLVM cannot complete Type construction without its result carrier."_view);
  }

  llvm::LLVMContext& context = get_context(*target);
  llvm::Module& module = get_module(*target);
  Bool sret = uses_memory_abi(*target, *llvm::unwrap(*result));
  Memory::Dynamic::Vector<llvm::Type*> native_parameters;
  if (sret) {
    native_parameters.insert(llvm::PointerType::getUnqual(context));
    record.sret_type = *result;
  }

  record.indirect_parameters.clear();
  for (const Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable>& retained :
       record.parameters.get_view()) {
    auto type = select_type(retained.get().get_type());
    auto native =
        type ? carriers->get_type(*type) : Core::Option<LLVMTypeRef>();
    if (!type || !native) {
      return fail_toolchain(
          program,
          "LLVM cannot complete Type construction without every Field "
          "carrier."_view);
    }
    Bool indirect = uses_memory_abi(*target, *llvm::unwrap(*native));
    record.indirect_parameters.insert(indirect);
    native_parameters.insert(
        indirect ? llvm::PointerType::getUnqual(context)
                 : llvm::unwrap(*native));
    native_parameters.insert(llvm::Type::getInt1Ty(context));
  }

  llvm::FunctionType* signature = llvm::FunctionType::get(
      sret ? llvm::Type::getVoidTy(context) : llvm::unwrap(*result),
      llvm::ArrayRef<llvm::Type*>(
          native_parameters.get_data(), native_parameters.get_size()),
      false);
  llvm::GlobalValue::LinkageTypes linkage =
      target->get_unit().is_package_member()
          ? llvm::GlobalValue::ExternalLinkage
          : llvm::GlobalValue::InternalLinkage;
  if (module.getNamedValue(llvm_text(record.symbol))) {
    return fail_toolchain(
        program,
        "LLVM Type construction symbol collides with another declaration."_view);
  }
  llvm::Function& function = *llvm::Function::Create(
      signature, linkage, llvm_text(record.symbol), module);
  if (target->get_unit().is_package_member()) {
    function.setVisibility(llvm::GlobalValue::HiddenVisibility);
  }

  Count offset = sret ? 1 : 0;
  if (sret) {
    function.addParamAttr(
        0,
        llvm::Attribute::getWithStructRetType(context, llvm::unwrap(*result)));
    function.addParamAttr(0, llvm::Attribute::NoAlias);
    function.addParamAttr(
        0, llvm::Attribute::getWithAlignment(
               context,
               module.getDataLayout().getABITypeAlign(llvm::unwrap(*result))));
  }
  for (Count index = 0; index < record.parameters.get_size(); index++) {
    auto type = select_type(record.parameters[index].get().get_type());
    auto native =
        type ? carriers->get_type(*type) : Core::Option<LLVMTypeRef>();
    BAIL_IF(!type || !native);
    Count parameter = offset + index * 2;
    if (record.indirect_parameters[index]) {
      function.addParamAttr(
          U32(parameter),
          llvm::Attribute::getWithByValType(context, llvm::unwrap(*native)));
      function.addParamAttr(
          U32(parameter), llvm::Attribute::getWithAlignment(
                              context, module.getDataLayout().getABITypeAlign(
                                           llvm::unwrap(*native))));
    }
    auto extension = get_extension(*carriers, *type);
    if (extension) {
      function.addParamAttr(U32(parameter), *extension);
    }
  }
  auto result_extension = get_extension(*carriers, owner);
  if (!sret && result_extension) {
    function.addRetAttr(*result_extension);
  }

  record.function = llvm::wrap(&function);
  if (record.provider && target->get_unit().is_package_member()) {
    target->add_publication(
        Tetrodotoxin::Terminal::Abi::Publication(owner, record.symbol));
  }
  return True;
}

static auto lower_construction_value(
    Llvm::Module::Body& body,
    const Llvm::Lowering::Execution& execution,
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Type& type,
    const Tetrodotoxin::Library::Language::Model::Pack& value)
    -> Core::Option<LLVMValueRef> {
  BAIL_IF(!execution.lower(value));
  auto lowered = body.find_values(value);
  BAIL_IF(!lowered);
  return carriers.fit_and_assemble(body, type, value, lowered->get_view());
}

auto Llvm::Module::Functions::lower_construction(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& owner,
    Core::View::Vector<ConstructionField> fields) const -> Bool {
  auto target = select_program(program);
  auto found = constructions.find(&owner);
  if (!target) {
    Core::Diagnostics::Log::error(
        "LLVM construction has no active module transaction."_view);
    return False;
  }
  if (!found) {
    Core::Diagnostics::Log::error(
        "LLVM construction was not reserved before emission."_view);
    return False;
  }
  if (!found->value.function) {
    Core::Diagnostics::Log::error(
        "LLVM construction was not completed before emission."_view);
    return fail_toolchain(
        program, "LLVM cannot lower Type construction before completion."_view);
  }

  ConstructionRecord& record = found->value;
  if (!record.provider) {
    return True;
  }

  // A Field fallback can prepare construction for another Type and grow this
  // map. Copy the completed signature facts before lowering begins so nested
  // construction cannot invalidate the active record.
  LLVMValueRef retained_function = *record.function;
  Core::Option<LLVMTypeRef> retained_sret_type = record.sret_type;
  Memory::Dynamic::Vector<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable>>
      retained_parameters = record.parameters;
  Memory::Dynamic::Vector<Bool> retained_indirect_parameters =
      record.indirect_parameters;

  llvm::Function& function =
      *llvm::cast<llvm::Function>(llvm::unwrap(retained_function));
  if (!function.empty()) {
    return True;
  }
  llvm::BasicBlock::Create(function.getContext(), "entry", &function);
  Core::Option<LLVMValueRef> sret;
  auto argument = function.arg_begin();
  if (retained_sret_type) {
    BAIL_IF(argument == function.arg_end());
    sret = llvm::wrap(&*argument);
    argument++;
  }

  Llvm::Module::Body native_body(
      *target, owner, retained_function, {}, sret, retained_sret_type);
  llvm::IRBuilder<>& native_builder = get_builder(native_body);
  Llvm::Lowering::Execution execution(native_body);
  const Carriers& carriers = target->get_carriers();
  Memory::Dynamic::Vector<LLVMValueRef> values;
  Count parameter_index = 0;
  for (const ConstructionField& input : fields) {
    const Tetrodotoxin::Source::Addressable& field = input.get_field();
    auto field_type = select_type(field.get_type());
    BAIL_IF(!field_type);

    Core::Option<LLVMValueRef> supplied;
    Core::Option<LLVMValueRef> present;
    if (input.is_parameter()) {
      BAIL_IF(
          parameter_index >= retained_parameters.get_size() ||
          &retained_parameters[parameter_index].get() != &field ||
          argument == function.arg_end());
      llvm::Value& native_value = *argument;
      argument++;
      auto carrier = carriers.get_type(*field_type);
      BAIL_IF(!carrier);
      supplied =
          retained_indirect_parameters[parameter_index]
              ? Core::Option<LLVMValueRef>(llvm::wrap(native_builder.CreateLoad(
                    llvm::unwrap(*carrier), &native_value)))
              : Core::Option<LLVMValueRef>(llvm::wrap(&native_value));
      BAIL_IF(argument == function.arg_end());
      present = llvm::wrap(&*argument);
      argument++;
      parameter_index++;
    }

    if (!supplied || !present) {
      auto lowered = lower_construction_value(
          native_body, execution, carriers, *field_type, input.get_fallback());
      if (!lowered) {
        Core::Diagnostics::Log::Message<256> message(
            Core::Diagnostics::Log::Level::Error, Core::Diagnostics::Source());
        message << "LLVM could not lower default construction for Field `"_view
                << field.get_name() << "`."_view;
        return False;
      }
      values.insert(*lowered);
      continue;
    }

    llvm::BasicBlock& supplied_block = *llvm::BasicBlock::Create(
        function.getContext(), "construction.supplied", &function);
    llvm::BasicBlock& fallback_block = *llvm::BasicBlock::Create(
        function.getContext(), "construction.default", &function);
    llvm::BasicBlock& merge_block = *llvm::BasicBlock::Create(
        function.getContext(), "construction.merge", &function);
    native_builder.CreateCondBr(
        llvm::unwrap(*present), &supplied_block, &fallback_block);

    native_builder.SetInsertPoint(&supplied_block);
    BAIL_IF(!native_body.acquire(*field_type, *supplied));
    native_builder.CreateBr(&merge_block);

    native_builder.SetInsertPoint(&fallback_block);
    auto lowered = lower_construction_value(
        native_body, execution, carriers, *field_type, input.get_fallback());
    BAIL_IF(!lowered || !native_body.acquire(*field_type, *lowered));
    llvm::BasicBlock* fallback_end = native_builder.GetInsertBlock();
    BAIL_IF(!fallback_end || fallback_end->getTerminator());
    native_builder.CreateBr(&merge_block);

    native_builder.SetInsertPoint(&merge_block);
    llvm::PHINode& selected = *native_builder.CreatePHI(
        llvm::unwrap(*supplied)->getType(), 2, "construction.value");
    selected.addIncoming(llvm::unwrap(*supplied), &supplied_block);
    selected.addIncoming(llvm::unwrap(*lowered), fallback_end);
    // Both branches contribute one owned value before they meet. Tracking the
    // merged carrier lets aggregate construction transfer that ownership once
    // without referring to an instruction confined to either predecessor.
    native_body.mark_owned(*field_type, llvm::wrap(&selected));
    values.insert(llvm::wrap(&selected));
  }
  if (parameter_index != retained_parameters.get_size() ||
      argument != function.arg_end()) {
    Core::Diagnostics::Log::error(
        "LLVM construction parameters did not consume their native signature."_view);
    return False;
  }

  auto constructed = carriers.construct(native_body, owner, values.get_view());
  if (!constructed) {
    Core::Diagnostics::Log::error(
        "LLVM could not assemble the construction result carrier."_view);
    return fail_toolchain(
        program, "LLVM could not assemble a constructed Type value."_view);
  }
  if (!native_body.acquire(owner, *constructed) ||
      !native_body.emit_storage_cleanup(0) ||
      !native_body.clear_temporary_cleanup() ||
      !native_body.create_return(*constructed)) {
    Core::Diagnostics::Log::error(
        "LLVM could not transfer construction result ownership."_view);
    return fail_toolchain(
        program, "LLVM could not finish constructed Type ownership."_view);
  }
  return True;
}

static auto select_construction_argument(
    const Tetrodotoxin::Library::Language::Model::Pack& arguments,
    Core::View::Bytes name) -> Core::Option<Count> {
  const Tetrodotoxin::Source::Layout& layout = arguments.get_layout();
  Core::Option<Count> selected;
  for (Count index = 0; index < layout.get_size(); index++) {
    auto candidate_name = layout.get_name(index);
    if (!candidate_name || *candidate_name != name) {
      continue;
    }
    BAIL_IF(selected || !layout.get_abstract(index));
    selected = index;
  }
  return selected;
}

auto Llvm::Module::Functions::call_construction(
    Llvm::Module::Emission& body,
    const Library::Language::Model::Pack& result,
    const Tetrodotoxin::Source::Type& owner,
    const Library::Language::Model::Pack& arguments) const -> Bool {
  auto native_body = select_body(body);
  auto found = constructions.find(&owner);
  if (!native_body || !found || !found->value.function) {
    return fail_toolchain(
        get_program(body),
        "LLVM cannot call Type construction before completion."_view);
  }

  ConstructionRecord& record = found->value;
  LLVMValueRef retained_function = *record.function;
  Core::Option<LLVMTypeRef> retained_sret_type = record.sret_type;
  Memory::Dynamic::Vector<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable>>
      retained_parameters = record.parameters;
  Memory::Dynamic::Vector<Bool> retained_indirect_parameters =
      record.indirect_parameters;
  const Carriers& carriers = native_body->get_program().get_carriers();
  Body::NativeValues native_arguments(retained_parameters.get_size() * 2 + 1);
  Core::Option<LLVMValueRef> returned_storage;
  if (retained_sret_type) {
    returned_storage = native_body->create_entry_alloca(
        *retained_sret_type, "construction.result"_view);
    BAIL_IF(!returned_storage);
    native_arguments.insert(*returned_storage);
  }

  for (Count index = 0; index < retained_parameters.get_size(); index++) {
    const Tetrodotoxin::Source::Addressable& field = retained_parameters[index].get();
    auto field_type = select_type(field.get_type());
    BAIL_IF(!field_type);
    auto selected = select_construction_argument(arguments, field.get_name());
    Core::Option<LLVMValueRef> native;
    if (selected) {
      auto lowered = native_body->find_values(arguments);
      if (lowered && *selected < lowered->get_size()) {
        const Core::Static::Vector<LLVMValueRef, 1> value = {{
          lowered->get_data()[*selected],
        }};
        native = carriers.fit_and_assemble(
            *native_body, *field_type, arguments, value);
      }
    } else {
      native = carriers.zero(native_body->get_program(), *field_type);
    }
    BAIL_IF(!native);

    LLVMValueRef argument = *native;
    if (retained_indirect_parameters[index]) {
      auto carrier = carriers.get_type(*field_type);
      BAIL_IF(!carrier);
      auto storage = native_body->create_entry_alloca(
          *carrier, "construction.argument"_view);
      BAIL_IF(
          !storage ||
          !LLVMBuildStore(native_body->get_builder(), *native, storage));
      argument = storage;
    }
    native_arguments.insert(argument);
    native_arguments.insert(LLVMConstInt(
        LLVMInt1TypeInContext(&native_body->get_program().get_context()),
        selected ? 1 : 0, 0));
  }

  LLVMTypeRef signature = LLVMGlobalGetValueType(retained_function);
  LLVMTypeRef native_result = LLVMGetReturnType(signature);
  Bool returns_void = Bool(LLVMGetTypeKind(native_result) == LLVMVoidTypeKind);
  LLVMValueRef invoked = LLVMBuildCall2(
      native_body->get_builder(), signature, retained_function,
      native_arguments.get_data(), U32(native_arguments.get_size()),
      returns_void ? "" : "construction");
  BAIL_IF(!invoked);
  LLVMValueRef returned =
      returned_storage ? LLVMBuildLoad2(
                             native_body->get_builder(), *retained_sret_type,
                             *returned_storage, "construction.value")
                       : invoked;
  BAIL_IF(!returned);
  native_body->mark_owned(owner, returned);
  Core::Static::Vector<LLVMValueRef, 1> values = {{returned}};
  return native_body->publish_values(result, values.get_view());
}

auto Llvm::Module::Functions::complete(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Callable& callable) const -> Bool {
  auto found = records.find(&callable);
  if (!found) {
    return fail_toolchain(
        program,
        "LLVM cannot complete a Callable before its owner reserves it."_view);
  }

  Record& record = found->value;
  if (record.function) {
    return True;
  }

  auto target = select_program(program);
  auto carriers = select_carriers(program);
  if (!target || !carriers) {
    return False;
  }

  Bool c_boundary = Bool(record.kind == Kind::Foreign);
  Bool c_publication = False;
  Bool package_publication = False;
  if ((record.kind == Kind::Function || record.kind == Kind::External) &&
      record.definition) {
    if (record.kind == Kind::Function) {
      c_publication = Bool(target->get_interface().find_export(callable));
      package_publication =
          Bool(target->get_unit().is_package_member() && c_publication);
    }
  } else if (record.kind == Kind::Function) {
    return fail_toolchain(
        program, "LLVM Function lowering lost its declaration owner."_view);
  }
  c_boundary = Bool(
      c_boundary || record.kind == Kind::External || c_publication ||
      package_publication);

  Core::View::Bytes selected_symbol = record.symbol;
  if (record.kind == Kind::Function) {
    auto exported = target->get_interface().find_export(callable);
    if (exported) {
      selected_symbol = exported->get_symbol();
    } else {
      Tetrodotoxin::Terminal::Abi::Symbol generated(
          target->get_arena(), callable,
          declares_self(callable)
              ? Tetrodotoxin::Terminal::Abi::Symbol::Kind::FunctionSelf
              : Tetrodotoxin::Terminal::Abi::Symbol::Kind::FunctionStatic,
          target->get_unit());
      selected_symbol = generated.get_view();
    }
  }

  llvm::Module& module = get_module(*target);
  if (module.getNamedValue(llvm_text(selected_symbol))) {
    return fail_callable(
        program, record.definition,
        "The native symbol collides with another emitted declaration."_view,
        "Choose a different `@symbol` spelling or declaration path."_view);
  }

  Memory::Dynamic::Vector<llvm::Type*> parameter_types;
  Memory::Dynamic::Vector<const Tetrodotoxin::Source::Type*> semantic_parameters;
  Memory::Dynamic::Vector<llvm::Type*> result_types;
  Memory::Dynamic::Vector<const Tetrodotoxin::Source::Type*> semantic_results;
  if (!select_parameter_types(
          program, *carriers, callable, parameter_types, semantic_parameters)) {
    return False;
  }

  if (!select_result_types(
          program, *carriers, callable, result_types, semantic_results)) {
    return False;
  }

  llvm::LLVMContext& context = get_context(*target);
  llvm::Type* result = llvm::Type::getVoidTy(context);
  if (result_types.get_size() == 1) {
    result = result_types[0];
  } else if (result_types.get_size() != 0) {
    result = llvm::StructType::get(
        context, llvm::ArrayRef<llvm::Type*>(
                     result_types.get_data(), result_types.get_size()));
  }

  record.result_type = llvm::wrap(result);
  llvm::Type* abi_result = result;
  Bool sret = Bool(c_boundary && uses_memory_abi(*target, *result));
  Memory::Dynamic::Vector<llvm::Type*> native_parameters;
  if (sret) {
    native_parameters.insert(llvm::PointerType::getUnqual(context));
    record.sret_type = llvm::wrap(result);
  } else if (c_boundary && result->isAggregateType()) {
    auto selected = CAbi::select_direct_type(program, llvm::wrap(result));
    if (!selected) {
      return fail_callable(
          program, record.definition,
          "The native C ABI cannot classify one compact Callable result."_view);
    }
    abi_result = llvm::unwrap(*selected);
  }
  record.result_abi_type = llvm::wrap(abi_result);

  record.indirect_parameters.clear();
  record.parameter_types.clear();
  record.parameter_abi_types.clear();
  record.parameter_abi_counts.clear();
  Count integer_registers = sret ? 1 : 0;
  Count sse_registers = 0;
  for (Count index = 0; index < parameter_types.get_size(); index++) {
    llvm::Type* parameter = parameter_types[index];
    Bool self_reference = Bool(index == 0 && declares_self(callable));
    Bool indirect = self_reference ||
                    Bool(c_boundary && uses_memory_abi(*target, *parameter));
    llvm::Type* abi_parameter = parameter;
    if (c_boundary && !indirect && parameter->isAggregateType()) {
      auto selected = CAbi::select_direct_type(program, llvm::wrap(parameter));
      if (!selected) {
        return fail_callable(
            program, record.definition,
            "The native C ABI cannot classify one compact Callable parameter."_view);
      }
      abi_parameter = llvm::unwrap(*selected);
    }

    Count required_integers = 0;
    Count required_sse = 0;
    if (c_boundary && !indirect) {
      count_c_registers(*abi_parameter, required_integers, required_sse);
      if (parameter->isAggregateType() &&
          (integer_registers + required_integers > 6 ||
           sse_registers + required_sse > 8)) {
        indirect = True;
        abi_parameter = parameter;
        required_integers = 0;
        required_sse = 0;
      }
    } else if (c_boundary && self_reference) {
      required_integers = 1;
    }

    record.indirect_parameters.insert(indirect);
    record.parameter_types.insert(llvm::wrap(parameter));
    record.parameter_abi_types.insert(llvm::wrap(abi_parameter));
    Count abi_count = 1;
    if (indirect) {
      native_parameters.insert(llvm::PointerType::getUnqual(context));
    } else if (
        c_boundary && parameter->isAggregateType() &&
        get_module(*target)
                .getDataLayout()
                .getTypeAllocSize(parameter)
                .getFixedValue() > 8) {
      auto* chunks = llvm::dyn_cast<llvm::StructType>(abi_parameter);
      if (!chunks || chunks->getNumElements() != 2) {
        return fail_callable(
            program, record.definition,
            "The native C ABI produced an invalid two register parameter."_view);
      }
      abi_count = chunks->getNumElements();
      for (llvm::Type* chunk : chunks->elements()) {
        native_parameters.insert(chunk);
      }
    } else {
      native_parameters.insert(abi_parameter);
    }
    record.parameter_abi_counts.insert(abi_count);
    integer_registers += required_integers;
    sse_registers += required_sse;
  }

  llvm::FunctionType* signature = llvm::FunctionType::get(
      sret ? llvm::Type::getVoidTy(context) : abi_result,
      llvm::ArrayRef<llvm::Type*>(
          native_parameters.get_data(), native_parameters.get_size()),
      false);
  llvm::GlobalValue::LinkageTypes linkage =
      record.kind == Kind::Foreign || record.kind == Kind::External ||
              c_publication || package_publication
          ? llvm::GlobalValue::ExternalLinkage
          : llvm::GlobalValue::InternalLinkage;
  llvm::Function& function = *llvm::Function::Create(
      signature, linkage, llvm_text(selected_symbol), module);
  if (record.kind == Kind::External || package_publication) {
    function.setVisibility(llvm::GlobalValue::HiddenVisibility);
  }
  Count parameter_offset = sret ? 1 : 0;
  if (sret) {
    function.addParamAttr(
        0, llvm::Attribute::getWithStructRetType(context, result));
    function.addParamAttr(0, llvm::Attribute::NoAlias);
    function.addParamAttr(
        0, llvm::Attribute::getWithAlignment(
               context, module.getDataLayout().getABITypeAlign(result)));
  }

  Count native_parameter = parameter_offset;
  for (Count index = 0; index < parameter_types.get_size(); index++) {
    Bool self_reference = Bool(index == 0 && declares_self(callable));
    if (record.indirect_parameters[index] && !self_reference) {
      function.addParamAttr(
          U32(native_parameter),
          llvm::Attribute::getWithByValType(context, parameter_types[index]));
      function.addParamAttr(
          U32(native_parameter),
          llvm::Attribute::getWithAlignment(
              context,
              module.getDataLayout().getABITypeAlign(parameter_types[index])));
    }

    if (record.parameter_abi_counts[index] == 1 &&
        record.parameter_abi_types[index] == record.parameter_types[index]) {
      auto extension = get_extension(*carriers, *semantic_parameters[index]);
      if (extension) {
        function.addParamAttr(U32(native_parameter), *extension);
      }
    }
    native_parameter += record.parameter_abi_counts[index];
  }

  auto library_callable =
      callable.select<Tetrodotoxin::Library::Language::Model::Callable>();
  if (semantic_results.get_size() == 1 &&
      !(library_callable && library_callable->get_self_result()) &&
      record.result_abi_type == record.result_type) {
    auto extension = get_extension(*carriers, *semantic_results[0]);
    if (extension) {
      function.addRetAttr(*extension);
    }
  }

  record.abi = c_publication ? "C"_view : Core::View::Bytes();
  record.symbol = selected_symbol;
  record.function = llvm::wrap(&function);
  if (c_publication || package_publication) {
    target->add_export(
        Tetrodotoxin::Terminal::Abi::Export(callable, selected_symbol));
  }
  if (target->get_unit().is_package_member() &&
      (c_publication || package_publication)) {
    target->add_publication(
        Tetrodotoxin::Terminal::Abi::Publication(callable, selected_symbol));
  }

  return True;
}

auto Llvm::Module::Functions::begin_body(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Callable& callable) const -> Core::Option<Lowering> {
  auto found = records.find(&callable);
  auto target = select_program(program);
  if (!found || !target || found->value.kind != Kind::Function ||
      !found->value.function) {
    fail_toolchain(
        program,
        "LLVM cannot begin a Body for this Callable declaration."_view);
    return {};
  }

  Record& record = found->value;
  llvm::Function& function =
      *llvm::cast<llvm::Function>(llvm::unwrap(*record.function));
  if (!function.empty()) {
    fail_callable(
        program, record.definition,
        "LLVM cannot lower one Callable Body more than once."_view);
    return {};
  }

  Core::Option<LLVMValueRef> sret;
  if (record.sret_type) {
    auto argument = function.arg_begin();
    if (argument == function.arg_end()) {
      fail_toolchain(
          program,
          "LLVM lost the reserved result address for a Callable Body."_view);
      return {};
    }

    sret = llvm::wrap(&*argument);
  }

  const Tetrodotoxin::Source::Layout& parameters = callable.get_parameters();
  if (record.indirect_parameters.get_size() != parameters.get_size() ||
      record.parameter_types.get_size() != parameters.get_size() ||
      record.parameter_abi_types.get_size() != parameters.get_size() ||
      record.parameter_abi_counts.get_size() != parameters.get_size()) {
    fail_toolchain(
        program,
        "LLVM Callable parameter state does not match its completed Layout."_view);
    return {};
  }

  llvm::BasicBlock::Create(function.getContext(), "entry", &function);
  return Lowering(*record.function, callable, sret, record.sret_type);
}

auto Llvm::Module::Functions::bind_parameters(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Callable& callable) const -> Bool {
  auto native_body = select_body(body);
  auto found = records.find(&callable);
  if (!native_body || !found || found->value.kind != Kind::Function ||
      !found->value.function ||
      native_body->get_function() != *found->value.function) {
    return False;
  }

  Record& record = found->value;
  llvm::Function& function =
      *llvm::cast<llvm::Function>(llvm::unwrap(*record.function));
  if (function.empty()) {
    return fail_toolchain(
        get_program(body),
        "LLVM cannot bind parameters without a Callable entry block."_view);
  }

  get_builder(*native_body).SetInsertPoint(&function.getEntryBlock());
  get_builder(*native_body).SetCurrentDebugLocation(llvm::DebugLoc());
  auto argument = function.arg_begin();
  if (record.sret_type) {
    argument++;
  }

  const Tetrodotoxin::Source::Layout& parameters = callable.get_parameters();
  for (Count index = 0; index < parameters.get_size(); index++) {
    auto entry_value = parameters.get_abstract(index);
    auto parameter = entry_value
                         ? entry_value->select<Tetrodotoxin::Source::Addressable>()
                         : Core::Option<const Tetrodotoxin::Source::Addressable&>();
    if (!parameter || argument == function.arg_end() ||
        record.parameter_abi_counts[index] == 0) {
      fail_toolchain(
          get_program(body),
          "LLVM cannot bind the completed Callable parameter Layout."_view);
      return False;
    }

    LLVMValueRef address;
    if (record.indirect_parameters[index]) {
      address = llvm::wrap(&*argument);
      argument++;
    } else {
      llvm::Type& abi_type = *llvm::unwrap(record.parameter_abi_types[index]);
      llvm::Value* native_value;
      if (record.parameter_abi_counts[index] == 1) {
        native_value = &*argument;
        argument++;
      } else {
        native_value = llvm::UndefValue::get(&abi_type);
        for (Count chunk = 0; chunk < record.parameter_abi_counts[index];
             chunk++) {
          if (argument == function.arg_end()) {
            return fail_toolchain(
                get_program(body),
                "LLVM lost one register from a compact C parameter."_view);
          }
          native_value =
              get_builder(*native_body)
                  .CreateInsertValue(
                      native_value, &*argument, U32(chunk), "parameter.chunk");
          argument++;
        }
      }

      auto semantic = CAbi::convert(
          *native_body, record.parameter_types[index], llvm::wrap(native_value),
          "parameter.value"_view);
      if (!semantic) {
        return fail_toolchain(
            get_program(body),
            "LLVM could not restore one semantic Callable parameter."_view);
      }
      LLVMValueRef storage = native_body->create_entry_alloca(
          record.parameter_types[index], "parameter"_view);
      get_builder(*native_body)
          .CreateStore(llvm::unwrap(*semantic), llvm::unwrap(storage));
      address = storage;
    }

    Bool published = native_body->publish_address(*parameter, address);
    if (!published) {
      fail_toolchain(
          get_program(body),
          "LLVM cannot publish one Callable parameter address twice."_view);
      return False;
    }
  }

  if (argument != function.arg_end()) {
    fail_toolchain(
        get_program(body),
        "LLVM Callable signature retains an unmatched native parameter."_view);
    return False;
  }

  return True;
}

auto Llvm::Module::Functions::end_body(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Callable& callable) const -> Bool {
  auto native_body = select_body(body);
  auto target = select_program(get_program(body));
  if (!native_body || !target) {
    return False;
  }

  auto retained_callable = native_body->get_callable();
  if (!retained_callable || &*retained_callable != &callable) {
    return target->fail_toolchain(
        "LLVM completed a Body under a different Callable identity."_view);
  }

  llvm::BasicBlock* block = get_builder(*native_body).GetInsertBlock();
  Bool completed = True;
  if (!block) {
    completed = target->fail_toolchain(
        "LLVM completed a Callable Body without an insertion block."_view);
  } else if (!block->getTerminator()) {
    auto library_callable =
        callable.select<Tetrodotoxin::Library::Language::Model::Callable>();
    auto self_result = library_callable
                           ? library_callable->get_self_result()
                           : Core::Option<const Tetrodotoxin::Source::Addressable&>();
    if (self_result) {
      auto address = native_body->find_address(*self_result);
      completed = Bool(
          address && llvm::unwrap(*address)->getType() ==
                         block->getParent()->getReturnType());
      if (completed) {
        completed = native_body->emit_storage_cleanup(0);
        completed &= native_body->emit_temporary_cleanup();
        completed &= native_body->create_return(*address);
      } else {
        completed = target->fail_toolchain(
            "LLVM could not return the fallthrough Self reference."_view);
      }
    } else if (callable.get_results().is_empty()) {
      completed = native_body->emit_storage_cleanup(0);
      completed &= native_body->emit_temporary_cleanup();
      completed &= native_body->create_return();
    } else {
      auto found = records.find(&callable);
      completed = fail_callable(
          get_program(body),
          found ? found->value.definition
                : Core::Option<const Tetrodotoxin::Language::Definition&>(),
          "A value returning Callable reached the end of its Body."_view,
          "Return the complete declared result Layout on every reachable path."_view);
    }
  }

  return completed;
}

auto Llvm::Module::Functions::append_call_arguments(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Callable& callable,
    Count parameter,
    LLVMValueRef value,
    Memory::Dynamic::Vector<LLVMValueRef>& arguments) const -> Bool {
  auto native_body = select_body(body);
  auto found = records.find(&callable);
  if (!native_body || !found ||
      parameter >= found->value.parameter_types.get_size() ||
      found->value.indirect_parameters[parameter] ||
      LLVMTypeOf(value) != found->value.parameter_types[parameter]) {
    return fail_toolchain(
        get_program(body),
        "LLVM cannot project this direct Callable argument into the C ABI."_view);
  }

  const Record& record = found->value;
  auto projected = CAbi::convert(
      *native_body, record.parameter_abi_types[parameter], value,
      "call.argument.abi"_view);
  if (!projected) {
    return False;
  }

  Count count = record.parameter_abi_counts[parameter];
  if (count == 1) {
    arguments.insert(*projected);
    return True;
  }

  auto* structure =
      llvm::dyn_cast<llvm::StructType>(llvm::unwrap(LLVMTypeOf(*projected)));
  if (!structure || structure->getNumElements() != count) {
    return fail_toolchain(
        get_program(body),
        "LLVM cannot split one compact C argument into its registers."_view);
  }
  for (Count index = 0; index < count; index++) {
    LLVMValueRef chunk = LLVMBuildExtractValue(
        native_body->get_builder(), *projected, U32(index), "call.argument");
    if (!chunk) {
      return False;
    }
    arguments.insert(chunk);
  }
  return True;
}

auto Llvm::Module::Functions::decode_call_result(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Callable& callable,
    LLVMValueRef value) const -> Core::Option<LLVMValueRef> {
  auto native_body = select_body(body);
  auto found = records.find(&callable);
  if (!native_body || !found || !found->value.result_type ||
      !found->value.result_abi_type || found->value.sret_type ||
      LLVMTypeOf(value) != *found->value.result_abi_type) {
    return {};
  }
  return CAbi::convert(
      *native_body, *found->value.result_type, value, "call.result.value"_view);
}

auto Llvm::Module::Functions::encode_return(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Callable& callable,
    LLVMValueRef value) const -> Core::Option<LLVMValueRef> {
  auto native_body = select_body(body);
  auto found = records.find(&callable);
  if (!native_body || !found || !found->value.result_type ||
      !found->value.result_abi_type || found->value.sret_type ||
      LLVMTypeOf(value) != *found->value.result_type) {
    return {};
  }
  return CAbi::convert(
      *native_body, *found->value.result_abi_type, value, "return.abi"_view);
}

auto Llvm::Module::Functions::find_function(
    const Tetrodotoxin::Source::Callable& callable) const -> Core::Option<LLVMValueRef> {
  auto found = records.find(&callable);
  return found ? found->value.function : Core::Option<LLVMValueRef>();
}

auto Llvm::Module::Functions::find_symbol(const Tetrodotoxin::Source::Callable& callable)
    const -> Core::Option<Core::View::Bytes> {
  auto found = records.find(&callable);
  return found && found->value.function
             ? Core::Option<Core::View::Bytes>(found->value.symbol)
             : Core::Option<Core::View::Bytes>();
}

auto Llvm::Module::Functions::find_sret_type(
    const Tetrodotoxin::Source::Callable& callable) const -> Core::Option<LLVMTypeRef> {
  auto found = records.find(&callable);
  return found ? found->value.sret_type : Core::Option<LLVMTypeRef>();
}

auto Llvm::Module::Functions::get_indirect_parameters(
    const Tetrodotoxin::Source::Callable& callable) const -> Core::View::Vector<Bool> {
  auto found = records.find(&callable);
  return found ? found->value.indirect_parameters.get_view()
               : Core::View::Vector<Bool>();
}

auto Llvm::Module::Functions::get_foreign_callables() const
    -> Core::View::Vector<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Callable>> {
  return foreign_callables.get_view();
}
