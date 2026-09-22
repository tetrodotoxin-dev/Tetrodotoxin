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
#include "tetrodotoxin/terminal/llvm/emission/states.hpp"
#include "tetrodotoxin/terminal/llvm/module/carriers.hpp"
#include "tetrodotoxin/terminal/llvm/module/functions.hpp"
#include "tetrodotoxin/terminal/llvm/module/globals.hpp"
#include "tetrodotoxin/terminal/llvm/module/literals.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library;

// Literals materialize constants with the carrier already reserved by their
// exact semantic Type.

class LiteralSelection {
 public:
  constexpr LiteralSelection(
      Llvm::Module::Body& body,
      Llvm::Module::Program& program,
      const Llvm::Module::Carriers& carriers)
      : body(body), program(program), carriers(carriers) {}

  Llvm::Module::Body& body;
  Llvm::Module::Program& program;
  const Llvm::Module::Carriers& carriers;
};

static auto select_literal_target(Llvm::Module::Body& body)
    -> Core::Option<LiteralSelection> {
  return LiteralSelection(
      body, body.get_program(), body.get_program().get_carriers());
}

static auto publish_literal(
    Llvm::Module::Body& body,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    LLVMValueRef value) -> Bool {
  BAIL_IF(!value);

  Core::Static::Vector<LLVMValueRef, 1> values = {{value}};
  return body.publish_values(result, values.get_view());
}

auto Llvm::Emission::States::unsigned_value(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    U64 value) const -> Bool {
  auto selected = select_literal_target(body);
  BAIL_IF(!selected);

  auto type = selected->carriers.get_type(carrier);
  BAIL_IF(!type);

  return publish_literal(selected->body, result, LLVMConstInt(*type, value, 0));
}

auto Llvm::Emission::States::signed_value(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    S64 value) const -> Bool {
  auto selected = select_literal_target(body);
  BAIL_IF(!selected);

  auto type = selected->carriers.get_type(carrier);
  BAIL_IF(!type);

  return publish_literal(
      selected->body, result, LLVMConstInt(*type, U64(value), 1));
}

auto Llvm::Emission::States::real_value(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    R64 value) const -> Bool {
  auto selected = select_literal_target(body);
  BAIL_IF(!selected);

  auto type = selected->carriers.get_type(carrier);
  BAIL_IF(!type);

  return publish_literal(selected->body, result, LLVMConstReal(*type, value));
}

auto Llvm::Emission::States::bytes_value(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    Core::View::Bytes value,
    Core::Option<const Tetrodotoxin::Language::Resource&> resource) const
    -> Bool {
  auto selected = select_literal_target(body);
  BAIL_IF(!selected);

  auto type = selected->carriers.get_type(carrier);
  BAIL_IF(!type);

  LLVMContextRef context = &selected->program.get_context();
  LLVMTypeKind kind = LLVMGetTypeKind(*type);
  if (kind == LLVMStructTypeKind) {
    auto bytes = Llvm::Module::Literals::create_bytes_view(
        selected->program, *type, value, resource);
    BAIL_IF(!bytes);
    return publish_literal(selected->body, result, *bytes);
  }

  if (kind != LLVMArrayTypeKind ||
      LLVMGetArrayLength2(*type) != value.get_size()) {
    return selected->program.fail_toolchain(
        "LLVM found incompatible byte Constant storage."_view);
  }

  LLVMValueRef constant = LLVMConstStringInContext2(
      context, reinterpret_cast<const char*>(value.get_data()),
      value.get_size(), 1);
  return publish_literal(selected->body, result, constant);
}

auto Llvm::Emission::States::object_value(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Library::Language::Model::Pack& result) const -> Bool {
  auto selected = select_literal_target(body);
  BAIL_IF(!selected);

  auto type = selected->carriers.get_type(carrier);
  BAIL_IF(!type || LLVMGetTypeKind(*type) != LLVMPointerTypeKind);
  return publish_literal(selected->body, result, LLVMConstNull(*type));
}

auto Llvm::Emission::States::enumeration_name(
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Source::Type& result_type,
    LLVMValueRef value,
    Core::View::Vector<U64> values,
    Core::View::Vector<Core::View::Bytes> names) const -> Bool {
  auto selected = select_literal_target(body);
  BAIL_IF(!selected || !value || values.get_size() != names.get_size());

  auto native_result = selected->carriers.get_type(result_type);
  Core::Option<llvm::StructType&> view_type;
  if (native_result) {
    auto selected =
        llvm::dyn_cast<llvm::StructType>(llvm::unwrap(*native_result));
    if (selected) {
      view_type = *selected;
    }
  }

  Core::Option<llvm::IntegerType&> native_value;
  auto selected_value =
      llvm::dyn_cast<llvm::IntegerType>(llvm::unwrap(LLVMTypeOf(value)));
  if (selected_value) {
    native_value = *selected_value;
  }
  BAIL_IF(!view_type || !native_value);

  auto& builder =
      *reinterpret_cast<llvm::IRBuilder<>*>(selected->body.get_builder());
  auto empty = Llvm::Module::Literals::create_bytes_view(
      selected->program, *native_result, {});
  BAIL_IF(!empty);
  llvm::Value* selected_data =
      builder.CreateExtractValue(llvm::unwrap(*empty), 0);
  llvm::Value* selected_size =
      builder.CreateExtractValue(llvm::unwrap(*empty), 1);
  for (Count remaining = values.get_size(); remaining != 0; remaining--) {
    Count index = remaining - 1;
    auto candidate = Llvm::Module::Literals::create_bytes_view(
        selected->program, *native_result, names[index]);
    BAIL_IF(!candidate);

    llvm::Value* matches = builder.CreateICmpEQ(
        llvm::unwrap(value),
        llvm::ConstantInt::get(&*native_value, values[index]),
        "enum.name.value");
    selected_data = builder.CreateSelect(
        matches, builder.CreateExtractValue(llvm::unwrap(*candidate), 0),
        selected_data, "enum.name.data");
    selected_size = builder.CreateSelect(
        matches, builder.CreateExtractValue(llvm::unwrap(*candidate), 1),
        selected_size, "enum.name.size");
  }

  llvm::Value* native = llvm::UndefValue::get(&*view_type);
  native = builder.CreateInsertValue(native, selected_data, 0);
  native = builder.CreateInsertValue(native, selected_size, 1);
  return publish_literal(selected->body, result, llvm::wrap(native));
}

// Short circuit logic returns its merge blocks to And or Or so no hidden
// operation state survives between their left and right inputs.

static auto logic_find_scalar(
    const Llvm::Module::Body& body,
    const Tetrodotoxin::Library::Language::Model::Pack& pack)
    -> Core::Option<LLVMValueRef> {
  auto values = body.find_values(pack);
  if (!values || values->get_size() != 1) {
    return {};
  }

  return values->get_data()[0];
}

auto Llvm::Emission::States::begin_logic(
    Logical operation,
    const Tetrodotoxin::Library::Language::Model::Pack& left) const
    -> Core::Option<Logic> {
  Llvm::Module::Body& native_body = body;
  auto left_value = logic_find_scalar(native_body, left);
  LLVMBasicBlockRef left_block = LLVMGetInsertBlock(native_body.get_builder());
  if (!left_value || !left_block) {
    return {};
  }

  LLVMContextRef context =
      LLVMGetModuleContext(LLVMGetGlobalParent(native_body.get_function()));
  LLVMBasicBlockRef right = LLVMAppendBasicBlockInContext(
      context, native_body.get_function(), "logical.right");
  LLVMBasicBlockRef merge = LLVMAppendBasicBlockInContext(
      context, native_body.get_function(), "logical.merge");
  if (!native_body.clear_temporary_cleanup()) {
    return {};
  }
  if (operation == Logical::And) {
    LLVMBuildCondBr(native_body.get_builder(), *left_value, right, merge);
  } else {
    LLVMBuildCondBr(native_body.get_builder(), *left_value, merge, right);
  }

  LLVMPositionBuilderAtEnd(native_body.get_builder(), right);
  return Logic(left_block, merge);
}

auto Llvm::Emission::States::end_logic(
    Logic state,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& left,
    const Tetrodotoxin::Library::Language::Model::Pack& right) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto left_value = logic_find_scalar(native_body, left);
  auto right_value = logic_find_scalar(native_body, right);
  LLVMBasicBlockRef right_block = LLVMGetInsertBlock(native_body.get_builder());
  if (!left_value || !right_value || !right_block) {
    return False;
  }

  BAIL_IF(!native_body.clear_temporary_cleanup());
  LLVMBuildBr(native_body.get_builder(), state.get_merge());
  LLVMPositionBuilderAtEnd(native_body.get_builder(), state.get_merge());
  LLVMValueRef selected =
      LLVMBuildPhi(native_body.get_builder(), LLVMTypeOf(*left_value), "");
  Core::Static::Vector<LLVMValueRef, 2> incoming_values = {{
    *left_value,
    *right_value,
  }};
  Core::Static::Vector<LLVMBasicBlockRef, 2> incoming_blocks = {{
    state.get_left(),
    right_block,
  }};
  LLVMAddIncoming(
      selected, incoming_values.get_data(), incoming_blocks.get_data(),
      U32(incoming_values.get_size()));

  Core::Static::Vector<LLVMValueRef, 1> values = {{selected}};
  return native_body.publish_values(result, values.get_view());
}

auto Llvm::Emission::States::logical_not(
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& operand) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto value = logic_find_scalar(native_body, operand);
  if (!value) {
    return False;
  }

  LLVMValueRef selected = LLVMBuildNot(native_body.get_builder(), *value, "");
  Core::Static::Vector<LLVMValueRef, 1> values = {{selected}};
  return native_body.publish_values(result, values.get_view());
}

// Option lowering follows the Perimortem inline payload and presence flag
// carrier. Only an engaged payload participates in ownership.

static auto option_select_carriers(const Llvm::Module::Body& body)
    -> Core::Option<const Llvm::Module::Carriers&> {
  return body.get_program().get_carriers();
}

static auto option_native_builder(const Llvm::Module::Body& body)
    -> llvm::IRBuilder<>& {
  return *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
}

static auto option_native_function(const Llvm::Module::Body& body)
    -> llvm::Function& {
  return *llvm::unwrap<llvm::Function>(body.get_function());
}

static auto publish_option(
    Llvm::Module::Body& body,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    LLVMValueRef value) -> Bool {
  Core::Static::Vector<LLVMValueRef, 1> values = {{value}};
  return body.publish_values(result, values.get_view());
}

auto Llvm::Emission::States::absent(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Library::Language::Model::Pack& result) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto value = carriers->zero(body.get_program(), carrier);
  return value && publish_option(native_body, result, *value);
}

auto Llvm::Emission::States::present(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Source::Type& element,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& payload) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto native_carrier = carriers->get_type(carrier);
  auto payload_values = native_body.find_values(payload);
  if (!native_carrier || !payload_values) {
    return False;
  }

  auto native_payload = carriers->fit_and_assemble(
      native_body, element, payload, payload_values->get_view());
  if (!native_payload || !native_body.acquire(element, *native_payload)) {
    return False;
  }

  if (carriers->is_object(element)) {
    native_body.mark_owned(carrier, *native_payload);
    return publish_option(native_body, result, *native_payload);
  }

  llvm::IRBuilder<>& builder = option_native_builder(native_body);
  llvm::Value& empty = *llvm::UndefValue::get(llvm::unwrap(*native_carrier));
  llvm::Value& with_payload =
      *builder.CreateInsertValue(&empty, llvm::unwrap(*native_payload), 0);
  llvm::Value& selected =
      *builder.CreateInsertValue(&with_payload, builder.getTrue(), 1);
  LLVMValueRef selected_handle = llvm::wrap(&selected);
  native_body.mark_owned(carrier, selected_handle);
  return publish_option(native_body, result, selected_handle);
}

auto Llvm::Emission::States::result(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& payload) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  auto payload_values = native_body.find_values(payload);
  if (!carriers || !payload_values) {
    return False;
  }

  auto selected = carriers->fit_and_assemble(
      native_body, carrier, payload, payload_values->get_view());
  return selected && publish_option(native_body, result, *selected);
}

auto Llvm::Emission::States::begin_unwrap(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Source::Type& element,
    const Tetrodotoxin::Library::Language::Model::Pack& option) const
    -> Core::Option<Choice> {
  Llvm::Module::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  if (!carriers) {
    return {};
  }

  auto native_option = native_body.find_value(option);
  auto native_carrier = carriers->get_type(carrier);
  if (!native_option || !native_carrier ||
      llvm::unwrap(*native_option)->getType() !=
          llvm::unwrap(*native_carrier)) {
    return {};
  }

  llvm::IRBuilder<>& builder = option_native_builder(native_body);
  llvm::Function& function = option_native_function(native_body);
  Bool niche = carriers->is_object(element);
  llvm::Value& present =
      niche ? *builder.CreateIsNotNull(llvm::unwrap(*native_option))
            : *builder.CreateExtractValue(llvm::unwrap(*native_option), 1);
  llvm::BasicBlock& payload_block = *llvm::BasicBlock::Create(
      function.getContext(), "option.payload", &function);
  llvm::BasicBlock& default_block = *llvm::BasicBlock::Create(
      function.getContext(), "option.default", &function);
  llvm::BasicBlock& merge = *llvm::BasicBlock::Create(
      function.getContext(), "option.merge", &function);
  builder.CreateCondBr(&present, &payload_block, &default_block);

  builder.SetInsertPoint(&payload_block);
  llvm::Value& payload =
      niche ? *llvm::unwrap(*native_option)
            : *builder.CreateExtractValue(llvm::unwrap(*native_option), 0);
  if (!carriers->retain(native_body, element, llvm::wrap(&payload))) {
    return {};
  }

  builder.CreateBr(&merge);
  llvm::BasicBlock* payload_end = builder.GetInsertBlock();
  if (!payload_end) {
    return {};
  }

  builder.SetInsertPoint(&default_block);
  return Choice(
      llvm::wrap(&payload), llvm::wrap(payload_end), llvm::wrap(&merge));
}

auto Llvm::Emission::States::end_unwrap(
    Choice state,
    const Tetrodotoxin::Source::Type& element,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& fallback) const
    -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto fallback_values = native_body.find_values(fallback);
  if (!fallback_values) {
    return False;
  }

  auto default_value = carriers->fit_and_assemble(
      native_body, element, fallback, fallback_values->get_view());
  if (!default_value || !native_body.acquire(element, *default_value)) {
    return False;
  }

  llvm::IRBuilder<>& builder = option_native_builder(native_body);
  builder.CreateBr(llvm::unwrap(state.get_merge()));
  llvm::BasicBlock* default_end = builder.GetInsertBlock();
  if (!default_end) {
    return False;
  }

  builder.SetInsertPoint(llvm::unwrap(state.get_merge()));
  llvm::PHINode& selected =
      *builder.CreatePHI(llvm::unwrap(*default_value)->getType(), 2);
  selected.addIncoming(
      llvm::unwrap(state.get_selected()),
      llvm::unwrap(state.get_selected_end()));
  selected.addIncoming(llvm::unwrap(*default_value), default_end);
  LLVMValueRef selected_handle = llvm::wrap(&selected);
  native_body.mark_owned(element, selected_handle);
  return publish_option(native_body, result, selected_handle);
}

auto Llvm::Emission::States::propagate_option(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Source::Type& element,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& option,
    const Tetrodotoxin::Library::Language::Model::Pack& escape) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto native_option = native_body.find_value(option);
  auto native_carrier = carriers->get_type(carrier);
  if (!native_option || !native_carrier ||
      llvm::unwrap(*native_option)->getType() !=
          llvm::unwrap(*native_carrier)) {
    return False;
  }

  llvm::IRBuilder<>& builder = option_native_builder(native_body);
  llvm::Function& function = option_native_function(native_body);
  llvm::BasicBlock& payload_block = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.payload", &function);
  llvm::BasicBlock& absent_block = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.absent", &function);
  llvm::BasicBlock& continued = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.continue", &function);
  Bool niche = carriers->is_object(element);
  llvm::Value& present =
      niche ? *builder.CreateIsNotNull(llvm::unwrap(*native_option))
            : *builder.CreateExtractValue(llvm::unwrap(*native_option), 1);
  builder.CreateCondBr(&present, &payload_block, &absent_block);

  builder.SetInsertPoint(&absent_block);
  if (!native_body.publish_values(escape, Core::View::Vector<LLVMValueRef>()) ||
      !Llvm::Emission::ControlFlow(native_body).escape_values(escape)) {
    return False;
  }

  builder.SetInsertPoint(&payload_block);
  llvm::Value& payload =
      niche ? *llvm::unwrap(*native_option)
            : *builder.CreateExtractValue(llvm::unwrap(*native_option), 0);
  if (!carriers->retain(native_body, element, llvm::wrap(&payload))) {
    return False;
  }

  builder.CreateBr(&continued);
  builder.SetInsertPoint(&continued);
  LLVMValueRef payload_handle = llvm::wrap(&payload);
  native_body.mark_owned(element, payload_handle);
  return publish_option(native_body, result, payload_handle);
}

auto Llvm::Emission::States::propagate_flag(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& flag,
    const Tetrodotoxin::Library::Language::Model::Pack& escape) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  auto native_flag = native_body.find_value(flag);
  auto native_carrier =
      carriers ? carriers->get_type(carrier) : Core::Option<LLVMTypeRef>();
  if (!carriers || !native_flag || !native_carrier ||
      !carriers->is_flag(carrier) ||
      LLVMTypeOf(*native_flag) != *native_carrier) {
    return False;
  }

  llvm::IRBuilder<>& builder = option_native_builder(native_body);
  llvm::Function& function = option_native_function(native_body);
  llvm::BasicBlock& continued = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.continue", &function);
  llvm::BasicBlock& inactive = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.inactive", &function);
  builder.CreateCondBr(llvm::unwrap(*native_flag), &continued, &inactive);

  builder.SetInsertPoint(&inactive);
  if (!native_body.publish_values(escape, Core::View::Vector<LLVMValueRef>()) ||
      !Llvm::Emission::ControlFlow(native_body).escape_values(escape)) {
    return False;
  }

  builder.SetInsertPoint(&continued);
  Core::Static::Vector<LLVMValueRef, 1> value = {{*native_flag}};
  return native_body.publish_values(result, value.get_view());
}

auto Llvm::Emission::States::propagate_result(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Source::Type& value,
    const Tetrodotoxin::Source::Type& error,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& source,
    const Tetrodotoxin::Library::Language::Model::Pack& escape) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  auto native_result = native_body.find_value(source);
  auto native_carrier =
      carriers ? carriers->get_type(carrier) : Core::Option<LLVMTypeRef>();
  if (!carriers || !native_result || !native_carrier ||
      LLVMTypeOf(*native_result) != *native_carrier) {
    return False;
  }

  llvm::IRBuilder<>& builder = option_native_builder(native_body);
  llvm::Function& function = option_native_function(native_body);
  llvm::Value& value_selected =
      *builder.CreateExtractValue(llvm::unwrap(*native_result), U32(1));
  llvm::BasicBlock& value_block = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.value", &function);
  llvm::BasicBlock& error_block = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.error", &function);
  llvm::BasicBlock& continued = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.continue", &function);
  builder.CreateCondBr(&value_selected, &value_block, &error_block);

  builder.SetInsertPoint(&error_block);
  auto native_error =
      carriers->select_result(native_body, carrier, *native_result, False);
  if (!native_error || !carriers->retain(native_body, error, *native_error)) {
    return False;
  }

  native_body.mark_owned(error, *native_error);
  Core::Static::Vector<LLVMValueRef, 1> escaped = {{*native_error}};
  if (!native_body.publish_values(escape, escaped.get_view()) ||
      !Llvm::Emission::ControlFlow(native_body).escape_values(escape)) {
    return False;
  }

  builder.SetInsertPoint(&value_block);
  auto native_value =
      carriers->select_result(native_body, carrier, *native_result, True);
  if (!native_value || !carriers->retain(native_body, value, *native_value)) {
    return False;
  }

  builder.CreateBr(&continued);
  builder.SetInsertPoint(&continued);
  native_body.mark_owned(value, *native_value);
  Core::Static::Vector<LLVMValueRef, 1> continued_value = {{*native_value}};
  return native_body.publish_values(result, continued_value.get_view());
}

// Sequence operations preserve contiguous pointer and length carriers while
// their semantic owners retain scalar or ranged selection state.

struct ContiguousSelection {
  llvm::Value& data;
  llvm::Value& length;
};
