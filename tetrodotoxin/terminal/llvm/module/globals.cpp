// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// The native bridge enters LLVM before the Perimortem owner so LLVM's standard
// declarations remain confined to this implementation unit.
#if defined(__cplusplus)
#include "llvm/IR/GlobalVariable.h"
#endif
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CBindingWrapping.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "perimortem/abi/core/cleanup.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/terminal/abi/symbol.hpp"
#include "tetrodotoxin/terminal/llvm/module/body.hpp"
#include "tetrodotoxin/terminal/llvm/module/carriers.hpp"
#include "tetrodotoxin/terminal/llvm/module/globals.hpp"
#include "tetrodotoxin/terminal/llvm/module/program.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

static auto get_target(Llvm::Module::Emission& program)
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

static auto get_carriers(Llvm::Module::Emission& program)
    -> Core::Option<const Llvm::Module::Carriers&> {
  auto native = get_target(program);
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
  auto target = get_target(program);
  return target ? target->fail_toolchain(message) : False;
}

static auto create_void_function(Llvm::Module::Program& program)
    -> LLVMValueRef {
  llvm::LLVMContext& context = get_context(program);
  llvm::Module& module = get_module(program);
  llvm::FunctionType& signature =
      *llvm::FunctionType::get(llvm::Type::getVoidTy(context), false);
  llvm::Function& function = *llvm::Function::Create(
      &signature, llvm::GlobalValue::InternalLinkage, "", module);
  return llvm::wrap(&function);
}

static auto emit_destructor(
    Llvm::Module::Program& program,
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Addressable& addressable,
    LLVMValueRef global) -> Core::Option<LLVMValueRef> {
  LLVMValueRef function = create_void_function(program);
  auto& native_function = *llvm::unwrap<llvm::Function>(function);
  llvm::BasicBlock::Create(
      native_function.getContext(), "entry", &native_function);
  Llvm::Module::Body body(program, addressable, function);
  llvm::IRBuilder<>& builder = get_builder(body);
  auto type = addressable.get_type().select<Tetrodotoxin::Source::Type>();
  auto native_type =
      type ? carriers.get_type(*type) : Core::Option<LLVMTypeRef>();
  if (!type || !native_type) {
    return {};
  }

  LLVMValueRef value = llvm::wrap(
      builder.CreateLoad(llvm::unwrap(*native_type), llvm::unwrap(global)));
  return carriers.release(body, *type, value) && body.create_return()
             ? Core::Option<LLVMValueRef>(function)
             : Core::Option<LLVMValueRef>();
}

static auto register_destructor(
    Llvm::Module::Program& program,
    Llvm::Module::Body& body,
    LLVMValueRef destructor) -> Bool {
  llvm::LLVMContext& context = get_context(program);
  llvm::Module& module = get_module(program);
  llvm::FunctionType& signature = *llvm::FunctionType::get(
      llvm::Type::getVoidTy(context), {llvm::PointerType::getUnqual(context)},
      false);
  llvm::FunctionCallee registration = module.getOrInsertFunction(
      llvm_text(Perimortem::Abi::Core::cleanup_register_symbol), &signature);

  get_builder(body).CreateCall(registration, {llvm::unwrap(destructor)});
  return True;
}

auto Llvm::Module::Globals::reserve(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Addressable& addressable,
    Record record) const -> Core::Option<Bool> {
  auto found = records.find(&addressable);
  if (found) {
    Bool same = Bool(
        found->value.has(Record::Property::Foreign) ==
            record.has(Record::Property::Foreign) &&
        found->value.abi == record.abi &&
        found->value.symbol == record.symbol &&
        found->value.has(Record::Property::Writable) ==
            record.has(Record::Property::Writable) &&
        found->value.has(Record::Property::External) ==
            record.has(Record::Property::External) &&
        found->value.has(Record::Property::Published) ==
            record.has(Record::Property::Published));
    if (!same) {
      fail_toolchain(
          program,
          "LLVM received different owners for one Addressable identity."_view);
      return {};
    }

    return False;
  }

  records.insert(&addressable, record);
  return True;
}

auto Llvm::Module::Globals::reserve_static(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Addressable& addressable) const -> Core::Option<Bool> {
  auto target = get_target(program);
  BAIL_IF(!target);

  auto field = addressable.select<Tetrodotoxin::Library::Language::Field>();
  Bool local = !field ||
               is_local_definition(target->get_unit(), field->get_definition());
  if (target->get_unit().is_package_member() && !local) {
    auto symbol = target->get_unit().find(addressable);
    BAIL_IF(!symbol);
    return reserve(
        program, addressable, Record(False, {}, *symbol, True, True, False));
  }

  auto publication = target->get_interface().find_publication(addressable);
  Tetrodotoxin::Terminal::Abi::Symbol generated(
      target->get_arena(), addressable,
      Tetrodotoxin::Terminal::Abi::Symbol::Kind::Address, target->get_unit());
  Bool published = Bool(publication);
  Core::View::Bytes symbol =
      publication ? publication->get_symbol() : generated.get_view();
  return reserve(
      program, addressable, Record(False, {}, symbol, True, False, published));
}

auto Llvm::Module::Globals::reserve_foreign(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Addressable& addressable,
    Core::View::Bytes abi,
    Core::View::Bytes symbol,
    Bool writable) const -> Core::Option<Bool> {
  if (abi != "C"_view || symbol.is_empty()) {
    fail_toolchain(
        program, "LLVM requires one named Foreign C State declaration."_view);
    return {};
  }

  auto reserved =
      reserve(program, addressable, Record(True, abi, symbol, writable));
  if (reserved && *reserved) {
    foreign_addressables.insert(addressable);
    auto target = get_target(program);
    if (!target ||
        !target->add_import(
            Tetrodotoxin::Linker::Import(
                writable ? Tetrodotoxin::Linker::Import::Kind::WritableState
                         : Tetrodotoxin::Linker::Import::Kind::ReadOnlyState,
                abi, symbol))) {
      return {};
    }
  }

  return reserved;
}

auto Llvm::Module::Globals::complete(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Addressable& addressable) const -> Bool {
  auto found = records.find(&addressable);
  auto target = get_target(program);
  auto carriers = get_carriers(program);
  if (!found || !target || !carriers) {
    return fail_toolchain(
        program,
        "LLVM cannot complete an Addressable before its owner reserves it."_view);
  }

  Record& record = found->value;
  if (record.global) {
    return True;
  }

  auto type = addressable.get_type().select<Tetrodotoxin::Source::Type>();
  auto native_type =
      type ? carriers->get_type(*type) : Core::Option<LLVMTypeRef>();
  if (!type || !native_type) {
    return fail_toolchain(
        program,
        "LLVM cannot find the completed carrier for an Addressable."_view);
  }

  llvm::Module& module = get_module(*target);
  if (module.getNamedValue(llvm_text(record.symbol))) {
    return fail_toolchain(
        program,
        "The native State symbol collides with another emitted declaration."_view);
  }

  llvm::Constant* inserted = module.getOrInsertGlobal(
      llvm_text(record.symbol), llvm::unwrap(*native_type));
  auto global = llvm::dyn_cast<llvm::GlobalVariable>(inserted);
  if (!global) {
    return fail_toolchain(
        program,
        "LLVM could not publish the completed Addressable carrier."_view);
  }

  global->setConstant(
      bool(
          record.has(Record::Property::Foreign) &&
          !record.has(Record::Property::Writable)));
  global->setLinkage(
      record.has(Record::Property::Foreign) ||
              record.has(Record::Property::External) ||
              record.has(Record::Property::Published)
          ? llvm::GlobalValue::ExternalLinkage
          : llvm::GlobalValue::InternalLinkage);
  if (record.has(Record::Property::External) ||
      record.has(Record::Property::Published)) {
    global->setVisibility(llvm::GlobalValue::HiddenVisibility);
  }
  if (!record.has(Record::Property::Foreign) &&
      !record.has(Record::Property::External)) {
    global->setInitializer(
        llvm::Constant::getNullValue(llvm::unwrap(*native_type)));
  }

  record.global = llvm::wrap(global);
  if (record.has(Record::Property::Published)) {
    target->add_publication(
        Tetrodotoxin::Terminal::Abi::Publication(addressable, record.symbol));
  }
  return True;
}

auto Llvm::Module::Globals::begin_initializer(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Addressable& addressable) const
    -> Core::Option<LLVMValueRef> {
  auto found = records.find(&addressable);
  auto target = get_target(program);
  if (!found || !target || found->value.has(Record::Property::Foreign) ||
      found->value.has(Record::Property::External) || !found->value.global ||
      found->value.initializer_function) {
    fail_toolchain(
        program, "LLVM cannot begin this Static initializer Body."_view);
    return {};
  }

  LLVMValueRef function = create_void_function(*target);
  auto& native_function = *llvm::unwrap<llvm::Function>(function);
  llvm::BasicBlock::Create(
      native_function.getContext(), "entry", &native_function);
  found->value.initializer_function = function;
  return function;
}

auto Llvm::Module::Globals::end_initializer(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Addressable& addressable,
    const Library::Language::Model::Pack& value) const -> Bool {
  auto native_body = body.get_kind() == Llvm::Module::Emission::Kind::Body
                         ? Core::Option<Llvm::Module::Body&>(
                               static_cast<Llvm::Module::Body&>(body))
                         : Core::Option<Llvm::Module::Body&>();
  auto target = get_target(get_program(body));
  auto carriers = get_carriers(get_program(body));
  auto found = records.find(&addressable);
  if (!native_body || !target || !carriers || !found ||
      &native_body->get_owner() != &addressable || !found->value.global ||
      !found->value.initializer_function ||
      native_body->get_function() != *found->value.initializer_function ||
      (get_builder(*native_body).GetInsertBlock() &&
       get_builder(*native_body).GetInsertBlock()->getTerminator())) {
    return fail_toolchain(
        get_program(body),
        "LLVM completed a Static initializer under different target state."_view);
  }

  auto type = addressable.get_type().select<Tetrodotoxin::Source::Type>();
  BAIL_IF(!type);

  auto values = native_body->find_values(value);
  auto assembled = values ? carriers->fit_and_assemble(
                                body, *type, value, values->get_view())
                          : Core::Option<LLVMValueRef>();
  Bool completed = Bool(assembled);
  if (completed) {
    completed = native_body->acquire(*type, *assembled);
  }

  if (completed) {
    get_builder(*native_body)
        .CreateStore(
            llvm::unwrap(*assembled), llvm::unwrap(*found->value.global));
    completed = native_body->clear_temporary_cleanup();
  }

  if (completed && carriers->owns_resources(*type)) {
    auto destructor =
        emit_destructor(*target, *carriers, addressable, *found->value.global);
    completed =
        destructor && register_destructor(*target, *native_body, *destructor);
  }

  if (completed) {
    completed = native_body->create_return();
  }

  if (completed) {
    auto function =
        llvm::unwrap<llvm::Function>(*found->value.initializer_function);
    llvm::appendToGlobalCtors(get_module(*target), function, 65535);
  }

  return completed;
}

auto Llvm::Module::Globals::find_address(
    const Tetrodotoxin::Source::Addressable& addressable) const
    -> Core::Option<LLVMValueRef> {
  auto found = records.find(&addressable);
  return found ? found->value.global : Core::Option<LLVMValueRef>();
}

auto Llvm::Module::Globals::find_symbol(
    const Tetrodotoxin::Source::Addressable& addressable) const
    -> Core::Option<Core::View::Bytes> {
  auto found = records.find(&addressable);
  return found && found->value.global
             ? Core::Option<Core::View::Bytes>(found->value.symbol)
             : Core::Option<Core::View::Bytes>();
}

auto Llvm::Module::Globals::permits_foreign_write(
    const Tetrodotoxin::Source::Addressable& addressable) const -> Bool {
  auto found = records.find(&addressable);
  return found && found->value.has(Record::Property::Foreign) &&
         found->value.has(Record::Property::Writable);
}

auto Llvm::Module::Globals::get_foreign_addressables() const
    -> Core::View::Vector<
        Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable>> {
  return foreign_addressables.get_view();
}
