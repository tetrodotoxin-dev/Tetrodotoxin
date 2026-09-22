// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// LLVM enters before Perimortem so the standard placement declaration is
// visible before the freestanding fallback used by Perimortem headers.
#if __has_include("llvm/IR/IRBuilder.h")
#include "llvm/IR/IRBuilder.h"
#else
#error LLVM IRBuilder is required by the Library native compiler
#endif

#include "perimortem/core/static/vector.hpp"

#include "llvm-c/Core.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "perimortem/abi/core/object.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/terminal/llvm/emission/control_flow.hpp"
#include "tetrodotoxin/terminal/llvm/module/carriers.hpp"
#include "tetrodotoxin/terminal/llvm/module/functions.hpp"
#include "tetrodotoxin/terminal/llvm/module/globals.hpp"
#include "tetrodotoxin/terminal/llvm/module/literals.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library;

// Arithmetic validates exact carriers before choosing signed, unsigned, or
// real LLVM instructions.

static auto control_select_carriers(const Llvm::Module::Body& body)
    -> Core::Option<const Llvm::Module::Carriers&> {
  return body.get_program().get_carriers();
}

static auto control_select_type(const Tetrodotoxin::Source::Abstract& answer)
    -> Core::Option<const Tetrodotoxin::Source::Type&> {
  auto direct = answer.select<Tetrodotoxin::Source::Type>();
  return direct ? direct : answer.resolve().select<Tetrodotoxin::Source::Type>();
}

static auto control_native_builder(const Llvm::Module::Body& body)
    -> llvm::IRBuilder<>& {
  return *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
}

static auto control_native_function(const Llvm::Module::Body& body)
    -> llvm::Function& {
  return *llvm::unwrap<llvm::Function>(body.get_function());
}

static auto control_find_value(
    const Llvm::Module::Body& body,
    const Tetrodotoxin::Library::Language::Model::Pack& pack)
    -> Core::Option<llvm::Value&> {
  auto found = body.find_value(pack);
  return found ? Core::Option<llvm::Value&>(*llvm::unwrap(*found))
               : Core::Option<llvm::Value&>();
}

static auto control_leave_loop(
    Llvm::Module::Body& body,
    const Tetrodotoxin::Source::Abstract& target,
    Bool breaking) -> Bool {
  Llvm::Module::Body& native_body = body;
  auto loop = native_body.find_loop(target);
  if (!loop || !native_body.emit_storage_cleanup(loop->get_storage_depth()) ||
      !native_body.emit_temporary_cleanup()) {
    return False;
  }

  LLVMBasicBlockRef destination =
      breaking ? loop->get_break_target() : loop->get_continue_target();
  control_native_builder(native_body).CreateBr(llvm::unwrap(destination));
  return True;
}

static auto control_return_values(
    Llvm::Module::Body& native_body,
    const Tetrodotoxin::Library::Language::Model::Pack& values,
    Bool preserve_tracking) -> Bool {
  auto carriers = control_select_carriers(native_body);
  if (!carriers) {
    return False;
  }

  auto callable = native_body.get_callable();
  auto returned = native_body.find_values(values);
  if (!callable || !returned) {
    return False;
  }

  const Tetrodotoxin::Source::Layout& results = callable->get_results();
  Core::Option<LLVMValueRef> native_return;
  if (results.is_empty()) {
    if (returned->get_size() != 0) {
      return False;
    }
  } else if (results.get_size() == 1) {
    auto entry = results.get_abstract(0);
    auto addressable = entry ? entry->select<Tetrodotoxin::Source::Addressable>()
                             : Core::Option<const Tetrodotoxin::Source::Addressable&>();
    auto type = addressable ? control_select_type(addressable->get_type())
                : entry     ? control_select_type(*entry)
                            : Core::Option<const Tetrodotoxin::Source::Type&>();
    // Named result slots carry values. Only the exact Self result identity
    // denotes a borrowed address, as it does in native signature generation.
    auto library_callable =
        callable->select<Tetrodotoxin::Library::Language::Model::Callable>();
    auto reference = library_callable
                         ? library_callable->get_self_result()
                         : Core::Option<const Tetrodotoxin::Source::Addressable&>();
    if (!type) {
      return False;
    }

    if (reference) {
      auto address = native_body.find_target_address(values);
      if (!address || &address->get_type() != &*type) {
        return False;
      }
      native_return = address->get_address();
    } else if (returned->get_size() == 0) {
      native_return = carriers->zero(native_body.get_program(), *type);
    } else {
      native_return = carriers->fit_and_assemble(
          native_body, *type, values, returned->get_view());
    }

    if (!native_return ||
        (!reference && !native_body.acquire(*type, *native_return))) {
      return False;
    }
  } else {
    if (returned->get_size() != results.get_size()) {
      return False;
    }

    auto fitted =
        carriers->fit(native_body, values, results, returned->get_view());
    if (!fitted) {
      return False;
    }

    Memory::Dynamic::Vector<LLVMValueRef> received(results.get_size());
    for (Count index = 0; index < results.get_size(); index++) {
      auto entry = results.get_abstract(index);
      auto addressable = entry ? entry->select<Tetrodotoxin::Source::Addressable>()
                               : Core::Option<const Tetrodotoxin::Source::Addressable&>();
      auto type = addressable ? control_select_type(addressable->get_type())
                  : entry     ? control_select_type(*entry)
                              : Core::Option<const Tetrodotoxin::Source::Type&>();
      if (!type) {
        return False;
      }

      Core::Static::Vector<LLVMValueRef, 1> source = {{
        fitted->get_data()[index],
      }};
      auto assembled =
          carriers->assemble(native_body, *type, source.get_view());
      if (!assembled || !native_body.acquire(*type, *assembled)) {
        return False;
      }

      received.insert(*assembled);
    }

    auto sret_type = native_body.get_sret_type();
    Memory::Dynamic::Vector<llvm::Type*> element_types(received.get_size());
    for (LLVMValueRef value : received.get_view()) {
      element_types.insert(llvm::unwrap(LLVMTypeOf(value)));
    }
    llvm::Type* aggregate_type =
        sret_type
            ? llvm::unwrap(*sret_type)
            : llvm::StructType::get(
                  control_native_function(native_body).getContext(),
                  llvm::ArrayRef<llvm::Type*>(
                      element_types.get_data(), element_types.get_size()));
    llvm::Value* aggregate = llvm::UndefValue::get(aggregate_type);
    for (Count index = 0; index < received.get_size(); index++) {
      aggregate = control_native_builder(native_body)
                      .CreateInsertValue(
                          aggregate, llvm::unwrap(received[index]), U32(index));
    }

    native_return = llvm::wrap(aggregate);
  }

  if (!native_body.emit_storage_cleanup(0) ||
      !(preserve_tracking ? native_body.emit_temporary_cleanup()
                          : native_body.clear_temporary_cleanup())) {
    return False;
  }

  return native_body.create_return(native_return);
}

auto Llvm::Emission::ControlFlow::return_values(
    const Tetrodotoxin::Library::Language::Model::Pack& values) const -> Bool {
  return control_return_values(body, values, False);
}

auto Llvm::Emission::ControlFlow::escape_values(
    const Tetrodotoxin::Library::Language::Model::Pack& values) const -> Bool {
  return control_return_values(body, values, True);
}

auto Llvm::Emission::ControlFlow::leave_loop(
    LoopAction action,
    const Tetrodotoxin::Source::Abstract& target) const -> Bool {
  return control_leave_loop(body, target, action == LoopAction::Break);
}

auto Llvm::Emission::ControlFlow::begin_branch(
    const Tetrodotoxin::Library::Language::Model::Pack& condition) const
    -> Core::Option<Branch> {
  Llvm::Module::Body& native_body = body;
  auto selected = control_find_value(native_body, condition);
  if (!selected) {
    return {};
  }

  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& branch =
      *llvm::BasicBlock::Create(function.getContext(), "if.body", &function);
  llvm::BasicBlock& alternate = *llvm::BasicBlock::Create(
      function.getContext(), "if.alternate", &function);
  llvm::BasicBlock& done =
      *llvm::BasicBlock::Create(function.getContext(), "if.done", &function);
  if (!native_body.clear_temporary_cleanup()) {
    return {};
  }
  builder.CreateCondBr(&*selected, &branch, &alternate);
  builder.SetInsertPoint(&branch);
  return Branch(llvm::wrap(&alternate), llvm::wrap(&done));
}

auto Llvm::Emission::ControlFlow::begin_alternate(Branch& state) const -> Bool {
  Llvm::Module::Body& native_body = body;
  if (state.has_alternate()) {
    return False;
  }

  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::BasicBlock* current = builder.GetInsertBlock();
  Bool reaches_done = Bool(current && !current->getTerminator());
  if (!native_body.clear_temporary_cleanup()) {
    return False;
  }
  if (reaches_done) {
    builder.CreateBr(llvm::unwrap(state.get_done()));
  }

  state.begin_alternate(reaches_done);
  builder.SetInsertPoint(llvm::unwrap(state.get_alternate()));
  return True;
}

auto Llvm::Emission::ControlFlow::end_branch(Branch state) const -> Bool {
  Llvm::Module::Body& native_body = body;
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::BasicBlock* current = builder.GetInsertBlock();
  Bool current_reaches = Bool(current && !current->getTerminator());
  if (!native_body.clear_temporary_cleanup()) {
    return False;
  }
  Bool body_reaches =
      state.has_alternate() ? state.body_reaches_done() : current_reaches;
  if (current_reaches) {
    builder.CreateBr(llvm::unwrap(state.get_done()));
  }

  Bool alternate_reaches = current_reaches;
  if (!state.has_alternate()) {
    builder.SetInsertPoint(llvm::unwrap(state.get_alternate()));
    builder.CreateBr(llvm::unwrap(state.get_done()));
    alternate_reaches = True;
  }

  builder.SetInsertPoint(llvm::unwrap(state.get_done()));
  if (!body_reaches && !alternate_reaches) {
    builder.CreateUnreachable();
  }

  return True;
}

auto Llvm::Emission::ControlFlow::begin_while(
    const Tetrodotoxin::Source::Abstract& owner) const -> Bool {
  Llvm::Module::Body& native_body = body;
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& condition = *llvm::BasicBlock::Create(
      function.getContext(), "while.condition", &function);
  llvm::BasicBlock& done =
      *llvm::BasicBlock::Create(function.getContext(), "while.done", &function);
  builder.CreateBr(&condition);
  builder.SetInsertPoint(&condition);
  return native_body.publish_loop(
      owner, llvm::wrap(&done), llvm::wrap(&condition),
      native_body.get_storage_depth());
}

auto Llvm::Emission::ControlFlow::select_while(
    const Tetrodotoxin::Source::Abstract& owner,
    const Tetrodotoxin::Library::Language::Model::Pack& condition) const
    -> Bool {
  Llvm::Module::Body& native_body = body;
  auto loop = native_body.find_loop(owner);
  auto selected = control_find_value(native_body, condition);
  if (!loop || !selected) {
    return False;
  }

  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& branch =
      *llvm::BasicBlock::Create(function.getContext(), "while.body", &function);
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  if (!native_body.clear_temporary_cleanup()) {
    return False;
  }
  builder.CreateCondBr(
      &*selected, &branch, llvm::unwrap(loop->get_break_target()));
  builder.SetInsertPoint(&branch);
  return True;
}

auto Llvm::Emission::ControlFlow::end_while(
    const Tetrodotoxin::Source::Abstract& owner) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto loop = native_body.find_loop(owner);
  if (!loop) {
    return False;
  }

  LLVMBasicBlockRef done = loop->get_break_target();
  LLVMBasicBlockRef condition = loop->get_continue_target();
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::BasicBlock* current = builder.GetInsertBlock();
  if (current && !current->getTerminator()) {
    builder.CreateBr(llvm::unwrap(condition));
  }

  if (!native_body.remove_loop(owner)) {
    return False;
  }

  builder.SetInsertPoint(llvm::unwrap(done));
  return True;
}

static auto select_enumeration_value(
    llvm::IRBuilder<>& builder,
    llvm::Value& index,
    llvm::IntegerType& type,
    Core::View::Vector<U64> values) -> llvm::Value& {
  llvm::Value* selected = llvm::ConstantInt::get(&type, 0);
  for (Count value_index = 0; value_index < values.get_size(); value_index++) {
    llvm::Value* matches = builder.CreateICmpEQ(
        &index, builder.getInt64(value_index), "enum.index");
    selected = builder.CreateSelect(
        matches, llvm::ConstantInt::get(&type, values[value_index]), selected,
        "enum.value");
  }

  return *selected;
}

static auto select_enumeration_name(
    Llvm::Module::Program& program,
    llvm::IRBuilder<>& builder,
    llvm::Value& index,
    llvm::StructType& type,
    Core::View::Vector<Core::View::Bytes> names) -> Core::Option<llvm::Value&> {
  auto empty =
      Llvm::Module::Literals::create_bytes_view(program, llvm::wrap(&type), {});
  BAIL_IF(!empty);

  llvm::Value* selected_data =
      builder.CreateExtractValue(llvm::unwrap(*empty), 0);
  llvm::Value* selected_size =
      builder.CreateExtractValue(llvm::unwrap(*empty), 1);
  for (Count name_index = 0; name_index < names.get_size(); name_index++) {
    auto candidate = Llvm::Module::Literals::create_bytes_view(
        program, llvm::wrap(&type), names[name_index]);
    BAIL_IF(!candidate);

    llvm::Value* matches = builder.CreateICmpEQ(
        &index, builder.getInt64(name_index), "enum.name.index");
    selected_data = builder.CreateSelect(
        matches, builder.CreateExtractValue(llvm::unwrap(*candidate), 0),
        selected_data, "enum.name.data");
    selected_size = builder.CreateSelect(
        matches, builder.CreateExtractValue(llvm::unwrap(*candidate), 1),
        selected_size, "enum.name.size");
  }

  llvm::Value* result = llvm::UndefValue::get(&type);
  result = builder.CreateInsertValue(result, selected_data, 0);
  result = builder.CreateInsertValue(result, selected_size, 1);
  return *result;
}

auto Llvm::Emission::ControlFlow::begin_sequence(
    const Tetrodotoxin::Source::Abstract& owner,
    const Tetrodotoxin::Source::Addressable& binding,
    const Tetrodotoxin::Source::Type& input_type,
    const Tetrodotoxin::Library::Language::Model::Pack& input) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = control_select_carriers(body);
  auto binding_type = control_select_type(binding.get_type());
  if (!carriers || !binding_type) {
    return False;
  }

  auto native_input = control_find_value(native_body, input);
  auto native_element = carriers->get_type(*binding_type);
  if (!native_input || !native_element) {
    return False;
  }

  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  Core::Option<llvm::Value&> start;
  Core::Option<llvm::Value&> end;
  Core::Option<llvm::Value&> data;
  Bool range = False;
  Count lifetime_depth = native_body.get_storage_depth();
  auto array = llvm::dyn_cast<llvm::ArrayType>(native_input->getType());
  auto structure = llvm::dyn_cast<llvm::StructType>(native_input->getType());
  if (array) {
    LLVMValueRef storage_handle =
        native_body.create_entry_alloca(llvm::wrap(array), "for.input"_view);
    llvm::Value* storage = llvm::unwrap(storage_handle);
    builder.CreateStore(&*native_input, storage);
    if (!native_body.acquire(input_type, llvm::wrap(&*native_input)) ||
        !native_body.register_storage(input_type, storage_handle)) {
      return False;
    }
    data = *builder.CreateInBoundsGEP(
        array, storage, {builder.getInt64(0), builder.getInt64(0)});
    start = *builder.getInt64(0);
    end = *builder.getInt64(array->getNumElements());
  } else if (
      structure && structure->getNumElements() == 2 &&
      structure->getElementType(0)->isPointerTy()) {
    data = *builder.CreateExtractValue(&*native_input, 0);
    start = *builder.getInt64(0);
    end = *builder.CreateExtractValue(&*native_input, 1);
  } else if (structure && structure->getNumElements() == 2) {
    start = *builder.CreateExtractValue(&*native_input, 0);
    end = *builder.CreateExtractValue(&*native_input, 1);
    range = True;
  } else {
    return False;
  }

  if (!start || !end || (!range && !data)) {
    return False;
  }
  // A Fixed input holding resources must outlive every iteration. Transfer that
  // value to loop storage, then clear receiver and evaluation temporaries once
  // in the preheader instead of emitting their cleanup inside the body.
  if (!native_body.clear_temporary_cleanup()) {
    return False;
  }

  LLVMValueRef binding_handle =
      native_body.create_entry_alloca(*native_element, "for.entry"_view);
  LLVMValueRef index_handle = native_body.create_entry_alloca(
      llvm::wrap(start->getType()), "for.index"_view);
  llvm::Value* binding_address = llvm::unwrap(binding_handle);
  llvm::Value* index_address = llvm::unwrap(index_handle);
  builder.CreateStore(&*start, index_address);
  if (!native_body.publish_address(binding, binding_handle)) {
    return False;
  }

  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& condition = *llvm::BasicBlock::Create(
      function.getContext(), "for.condition", &function);
  llvm::BasicBlock& branch =
      *llvm::BasicBlock::Create(function.getContext(), "for.body", &function);
  llvm::BasicBlock& step =
      *llvm::BasicBlock::Create(function.getContext(), "for.step", &function);
  llvm::BasicBlock& done =
      *llvm::BasicBlock::Create(function.getContext(), "for.done", &function);
  builder.CreateBr(&condition);

  builder.SetInsertPoint(&condition);
  llvm::Value* current = builder.CreateLoad(start->getType(), index_address);
  llvm::Value* active = range && carriers->is_signed(*binding_type)
                            ? builder.CreateICmpSLT(current, &*end)
                            : builder.CreateICmpULT(current, &*end);
  builder.CreateCondBr(active, &branch, &done);

  builder.SetInsertPoint(&branch);
  if (range) {
    builder.CreateStore(current, binding_address);
  } else {
    llvm::Value* address =
        builder.CreateGEP(llvm::unwrap(*native_element), &*data, current);
    llvm::Value* selected =
        builder.CreateLoad(llvm::unwrap(*native_element), address);
    builder.CreateStore(selected, binding_address);
  }

  llvm::IRBuilder<> step_builder(&step);
  llvm::Value* stepped =
      step_builder.CreateLoad(start->getType(), index_address);
  step_builder.CreateStore(
      step_builder.CreateAdd(
          stepped, llvm::ConstantInt::get(start->getType(), 1)),
      index_address);
  step_builder.CreateBr(&condition);
  return native_body.publish_loop(
      owner, llvm::wrap(&done), llvm::wrap(&step), lifetime_depth);
}

auto Llvm::Emission::ControlFlow::begin_enumeration(
    const Tetrodotoxin::Source::Abstract& owner,
    const Tetrodotoxin::Source::Layout& bindings,
    Core::View::Vector<U64> values,
    Core::View::Vector<Core::View::Bytes> names) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = control_select_carriers(body);
  if (!carriers || (bindings.get_size() != 1 && bindings.get_size() != 2) ||
      (!names.is_empty() && names.get_size() != values.get_size())) {
    return False;
  }

  auto value_entry = bindings.get_abstract(0);
  auto value_binding = value_entry
                           ? value_entry->select<Tetrodotoxin::Source::Addressable>()
                           : Core::Option<const Tetrodotoxin::Source::Addressable&>();
  auto value_semantic_type =
      value_binding ? control_select_type(value_binding->get_type())
                    : Core::Option<const Tetrodotoxin::Source::Type&>();
  auto value_type = value_semantic_type
                        ? carriers->get_type(*value_semantic_type)
                        : Core::Option<LLVMTypeRef>();
  Core::Option<llvm::IntegerType&> native_value;
  if (value_type) {
    auto selected =
        llvm::dyn_cast<llvm::IntegerType>(llvm::unwrap(*value_type));
    if (selected) {
      native_value = *selected;
    }
  }

  if (!value_binding || !native_value) {
    return False;
  }

  Count lifetime_depth = native_body.get_storage_depth();
  if (!native_body.clear_temporary_cleanup()) {
    return False;
  }

  Core::Option<const Tetrodotoxin::Source::Addressable&> name_binding;
  Core::Option<llvm::StructType&> name_type;
  if (bindings.get_size() == 2) {
    auto name_entry = bindings.get_abstract(1);
    name_binding = name_entry ? name_entry->select<Tetrodotoxin::Source::Addressable>()
                              : Core::Option<const Tetrodotoxin::Source::Addressable&>();
    auto name_semantic_type =
        name_binding ? control_select_type(name_binding->get_type())
                     : Core::Option<const Tetrodotoxin::Source::Type&>();
    auto native_name = name_semantic_type
                           ? carriers->get_type(*name_semantic_type)
                           : Core::Option<LLVMTypeRef>();
    Core::Option<llvm::StructType&> selected_name;
    if (native_name) {
      auto selected =
          llvm::dyn_cast<llvm::StructType>(llvm::unwrap(*native_name));
      if (selected) {
        selected_name = *selected;
      }
    }

    if (!name_binding || !selected_name ||
        names.is_empty() != values.is_empty()) {
      return False;
    }

    name_type = *selected_name;
  }

  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  LLVMValueRef index_handle = native_body.create_entry_alloca(
      llvm::wrap(builder.getInt64Ty()), "for.index"_view);
  LLVMValueRef value_handle =
      native_body.create_entry_alloca(*value_type, "for.value"_view);
  llvm::Value* index_address = llvm::unwrap(index_handle);
  llvm::Value* value_address = llvm::unwrap(value_handle);
  builder.CreateStore(builder.getInt64(0), index_address);
  if (!native_body.publish_address(*value_binding, value_handle)) {
    return False;
  }

  Core::Option<LLVMValueRef> name_handle;
  if (name_binding && name_type) {
    name_handle = native_body.create_entry_alloca(
        llvm::wrap(&*name_type), "for.name"_view);
    if (!native_body.publish_address(*name_binding, *name_handle)) {
      return False;
    }
  }

  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& condition = *llvm::BasicBlock::Create(
      function.getContext(), "for.condition", &function);
  llvm::BasicBlock& branch =
      *llvm::BasicBlock::Create(function.getContext(), "for.body", &function);
  llvm::BasicBlock& step =
      *llvm::BasicBlock::Create(function.getContext(), "for.step", &function);
  llvm::BasicBlock& done =
      *llvm::BasicBlock::Create(function.getContext(), "for.done", &function);
  builder.CreateBr(&condition);

  builder.SetInsertPoint(&condition);
  llvm::Value* current =
      builder.CreateLoad(builder.getInt64Ty(), index_address);
  llvm::Value* active =
      builder.CreateICmpULT(current, builder.getInt64(values.get_size()));
  builder.CreateCondBr(active, &branch, &done);

  builder.SetInsertPoint(&branch);
  llvm::Value& selected =
      select_enumeration_value(builder, *current, *native_value, values);
  builder.CreateStore(&selected, value_address);
  if (name_handle && name_type) {
    auto selected_name = select_enumeration_name(
        native_body.get_program(), builder, *current, *name_type, names);
    if (!selected_name) {
      return False;
    }

    builder.CreateStore(&*selected_name, llvm::unwrap(*name_handle));
  }

  llvm::IRBuilder<> step_builder(&step);
  llvm::Value* stepped =
      step_builder.CreateLoad(builder.getInt64Ty(), index_address);
  step_builder.CreateStore(
      step_builder.CreateAdd(stepped, step_builder.getInt64(1)), index_address);
  step_builder.CreateBr(&condition);
  return native_body.publish_loop(
      owner, llvm::wrap(&done), llvm::wrap(&step), lifetime_depth);
}

auto Llvm::Emission::ControlFlow::end_iteration(
    const Tetrodotoxin::Source::Abstract& owner) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto loop = native_body.find_loop(owner);
  if (!loop) {
    return False;
  }

  LLVMBasicBlockRef done = loop->get_break_target();
  LLVMBasicBlockRef step = loop->get_continue_target();
  Count lifetime_depth = loop->get_lifetime_depth();
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::BasicBlock* current = builder.GetInsertBlock();
  if (current && !current->getTerminator()) {
    builder.CreateBr(llvm::unwrap(step));
  }

  if (!native_body.remove_loop(owner)) {
    return False;
  }

  builder.SetInsertPoint(llvm::unwrap(done));
  Bool cleaned = native_body.emit_storage_cleanup(lifetime_depth);
  Bool resized = native_body.resize_storage(lifetime_depth);
  return cleaned && resized;
}

auto Llvm::Emission::ControlFlow::begin_match(
    const Tetrodotoxin::Library::Language::Model::Pack& input) const
    -> Core::Option<Match> {
  Llvm::Module::Body& native_body = body;
  auto native_input = native_body.find_value(input);
  if (!native_input) {
    return {};
  }

  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& done =
      *llvm::BasicBlock::Create(function.getContext(), "match.done", &function);
  return Match(*native_input, llvm::wrap(&done));
}

auto Llvm::Emission::ControlFlow::begin_constant_case(
    Match& state,
    const Tetrodotoxin::Library::Language::Model::Pack& constant) const
    -> Core::Option<MatchCase> {
  Llvm::Module::Body& native_body = body;
  auto selected_constant = control_find_value(native_body, constant);
  if (!selected_constant) {
    return {};
  }

  llvm::Value* input = llvm::unwrap(state.get_input());
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::Value* matches = input->getType()->isFloatingPointTy()
                             ? builder.CreateFCmpOEQ(input, &*selected_constant)
                             : builder.CreateICmpEQ(input, &*selected_constant);
  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& selected =
      *llvm::BasicBlock::Create(function.getContext(), "match.case", &function);
  llvm::BasicBlock& next =
      *llvm::BasicBlock::Create(function.getContext(), "match.next", &function);
  builder.CreateCondBr(matches, &selected, &next);
  builder.SetInsertPoint(&selected);
  return MatchCase(native_body.get_storage_depth(), llvm::wrap(&next));
}

auto Llvm::Emission::ControlFlow::begin_value_case(
    Match& state,
    const Tetrodotoxin::Source::Addressable& payload,
    Tetrodotoxin::Source::Lexical::Anchor) const -> Core::Option<MatchCase> {
  Llvm::Module::Body& native_body = body;
  auto carriers = control_select_carriers(body);
  if (!carriers) {
    return {};
  }

  auto type = control_select_type(payload.get_type());
  auto native_type =
      type ? carriers->get_type(*type) : Core::Option<LLVMTypeRef>();
  if (!type || !native_type) {
    return {};
  }

  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::Value* input = llvm::unwrap(state.get_input());
  llvm::Value* present = builder.CreateExtractValue(input, 1);
  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& selected =
      *llvm::BasicBlock::Create(function.getContext(), "match.case", &function);
  llvm::BasicBlock& next =
      *llvm::BasicBlock::Create(function.getContext(), "match.next", &function);
  builder.CreateCondBr(present, &selected, &next);
  builder.SetInsertPoint(&selected);

  Count storage_depth = native_body.get_storage_depth();
  LLVMValueRef address =
      native_body.create_entry_alloca(*native_type, "match.payload"_view);
  LLVMValueRef value = llvm::wrap(builder.CreateExtractValue(input, 0));
  if (!carriers->retain(native_body, *type, value)) {
    return {};
  }

  builder.CreateStore(llvm::unwrap(value), llvm::unwrap(address));
  if (!native_body.publish_address(payload, address) ||
      !native_body.register_storage(*type, address)) {
    return {};
  }

  return MatchCase(storage_depth, llvm::wrap(&next));
}

auto Llvm::Emission::ControlFlow::end_match_case(
    Match& state,
    MatchCase selected) const -> Bool {
  Llvm::Module::Body& native_body = body;
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::BasicBlock* current = builder.GetInsertBlock();
  Bool reaches_done = Bool(current && !current->getTerminator());
  Bool cleaned = !reaches_done ||
                 native_body.emit_storage_cleanup(selected.get_storage_depth());
  if (reaches_done && cleaned) {
    builder.CreateBr(llvm::unwrap(state.get_done()));
    state.set_reaches_done();
  }

  Bool resized = native_body.resize_storage(selected.get_storage_depth());
  auto next = selected.get_next();
  if (next) {
    builder.SetInsertPoint(llvm::unwrap(*next));
  }

  return cleaned && resized;
}

auto Llvm::Emission::ControlFlow::begin_default_case() const -> MatchCase {
  return MatchCase(body.get_storage_depth(), {});
}

auto Llvm::Emission::ControlFlow::end_match(
    Match state,
    Bool unmatched_reaches_next) const -> Bool {
  Llvm::Module::Body& native_body = body;
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::BasicBlock* current = builder.GetInsertBlock();
  Bool unmatched_active = Bool(current && !current->getTerminator());
  if (unmatched_active && unmatched_reaches_next) {
    builder.CreateBr(llvm::unwrap(state.get_done()));
    state.set_reaches_done();
  } else if (unmatched_active) {
    builder.CreateUnreachable();
  }

  builder.SetInsertPoint(llvm::unwrap(state.get_done()));
  if (!state.reaches_done()) {
    builder.CreateUnreachable();
  }

  return True;
}

auto Llvm::Emission::ControlFlow::end_block(
    const Tetrodotoxin::Source::Abstract& block) const -> Bool {
  auto scope = body.take_block_scope(block);
  if (!scope) {
    return False;
  }

  llvm::BasicBlock* selected = control_native_builder(body).GetInsertBlock();
  Bool cleaned = !selected || selected->getTerminator() ||
                 body.emit_storage_cleanup(scope->get_storage_depth());
  Bool resized = body.resize_storage(scope->get_storage_depth());
  Bool debug = !body.get_debug_scope() || body.pop_debug_scope();
  return cleaned && resized && debug;
}

auto Llvm::Emission::ControlFlow::end_statement() const -> Bool {
  return body.clear_temporary_cleanup();
}

auto Llvm::Emission::ControlFlow::bind_local(
    const Tetrodotoxin::Source::Addressable& local,
    const Tetrodotoxin::Library::Language::Model::Pack& value) const -> Bool {
  auto carriers = control_select_carriers(body);
  Llvm::Module::Program& program = body.get_program();
  if (!carriers) {
    return False;
  }

  auto type = control_select_type(local.get_type());
  auto native_type =
      type ? carriers->get_type(*type) : Core::Option<LLVMTypeRef>();
  auto values = body.find_values(value);
  if (!type || !native_type) {
    return program.fail_toolchain(
        "LLVM cannot bind a Local without its completed carrier."_view);
  }
  if (!values) {
    return program.fail_toolchain(
        "LLVM cannot bind a Local before its initializer values are published."_view);
  }

  auto stored =
      carriers->fit_and_assemble(body, *type, value, values->get_view());
  if (!stored || !body.acquire(*type, *stored)) {
    return program.fail_toolchain(
        "LLVM cannot transfer the selected value into one Local."_view);
  }

  LLVMValueRef address = body.create_entry_alloca(*native_type, "local"_view);
  control_native_builder(body).CreateStore(
      llvm::unwrap(*stored), llvm::unwrap(address));
  if (!body.publish_address(local, address) ||
      !body.register_storage(*type, address)) {
    return program.fail_toolchain(
        "LLVM cannot publish storage for one Local."_view);
  }
  return True;
}

// ControlFlow storage follows authored Block and Local lifetimes while
// temporary values are cleared at each completed Statement boundary.

auto Llvm::Emission::ControlFlow::begin_function(
    const Tetrodotoxin::Source::Callable& callable,
    const Tetrodotoxin::Language::Definition& definition) const -> Bool {
  return get_program().get_debug().begin_function(body, callable, definition);
}

auto Llvm::Emission::ControlFlow::parameter(
    const Tetrodotoxin::Source::Addressable& parameter,
    Tetrodotoxin::Source::Lexical::Anchor anchor,
    Count index) const -> Bool {
  return get_program().get_debug().parameter(body, parameter, anchor, index);
}

auto Llvm::Emission::ControlFlow::end_function() const -> Bool {
  return get_program().get_debug().end_function(body);
}

auto Llvm::Emission::ControlFlow::begin_block(
    const Tetrodotoxin::Source::Abstract& block,
    Tetrodotoxin::Source::Lexical::Anchor anchor) const -> Bool {
  return get_program().get_debug().begin_block(body, block, anchor);
}

auto Llvm::Emission::ControlFlow::statement(Tetrodotoxin::Source::Lexical::Anchor anchor) const
    -> Bool {
  return get_program().get_debug().statement(body, anchor);
}

auto Llvm::Emission::ControlFlow::local(
    const Tetrodotoxin::Source::Addressable& local,
    Tetrodotoxin::Source::Lexical::Anchor anchor) const -> Bool {
  return get_program().get_debug().local(body, local, anchor);
}

auto Llvm::Emission::ControlFlow::has_full_debug() const -> Bool {
  return get_program().get_debug().get_level() ==
         Llvm::Module::Debug::Level::Full;
}

auto Llvm::Emission::ControlFlow::constant_local(
    const Tetrodotoxin::Source::Addressable& local,
    const Tetrodotoxin::Library::Language::Model::Pack& value,
    Tetrodotoxin::Source::Lexical::Anchor anchor) const -> Bool {
  Llvm::Module::Program& program = get_program();
  if (program.get_debug().get_level() != Llvm::Module::Debug::Level::Full) {
    return True;
  }

  auto values = body.find_values(value);
  if (!values) {
    return program.fail_toolchain(
        "LLVM lost one const Local value before debug emission."_view);
  }

  const Llvm::Module::Carriers& carriers = program.get_carriers();
  auto type = control_select_type(local.get_type());
  BAIL_IF(!type);
  auto assembled =
      carriers.fit_and_assemble(body, *type, value, values->get_view());
  return assembled &&
         program.get_debug().value(body, local, anchor, *assembled);
}
