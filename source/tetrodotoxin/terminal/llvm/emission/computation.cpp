// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// LLVM must enter before Perimortem so the standard placement declaration is
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
#include "tetrodotoxin/terminal/llvm/emission/computation.hpp"
#include "tetrodotoxin/terminal/llvm/module/carriers.hpp"
#include "tetrodotoxin/terminal/llvm/module/functions.hpp"
#include "tetrodotoxin/terminal/llvm/module/globals.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library;

// Arithmetic validates exact carriers before choosing signed, unsigned, or
// real LLVM instructions.

static auto arithmetic_find_scalar(
    const Llvm::Module::Body& body,
    const Tetrodotoxin::Source::Pack& pack) -> Core::Option<LLVMValueRef> {
  auto values = body.find_values(pack);
  if (!values || values->get_size() != 1) {
    return {};
  }

  return values->get_data()[0];
}

static auto arithmetic_select_carriers(const Llvm::Module::Body& body)
    -> Core::Option<const Llvm::Module::Carriers&> {
  return body.get_program().get_carriers();
}

static auto arithmetic_has_native_carrier(
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Type& carrier,
    LLVMValueRef left,
    Core::Option<LLVMValueRef> right = {}) -> Bool {
  auto native = carriers.get_type(carrier);
  if (!native || LLVMTypeOf(left) != *native) {
    return False;
  }

  return !right || LLVMTypeOf(*right) == *native;
}

static auto arithmetic_publish_scalar(
    Llvm::Module::Body& body,
    const Tetrodotoxin::Source::Pack& result,
    LLVMValueRef value) -> Bool {
  Core::Static::Vector<LLVMValueRef, 1> native = {{value}};
  return body.publish_values(result, native.get_view());
}

auto Llvm::Emission::Computation::arithmetic(
    Arithmetic operation,
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Pack& left,
    const Tetrodotoxin::Source::Pack& right) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = arithmetic_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto left_value = arithmetic_find_scalar(native_body, left);
  auto right_value = arithmetic_find_scalar(native_body, right);
  if (!left_value || !right_value ||
      !arithmetic_has_native_carrier(
          *carriers, carrier, *left_value, *right_value)) {
    return False;
  }

  Bool real = carriers->is_real(carrier);
  Bool signed_value = carriers->is_signed(carrier);
  switch (operation) {
  case Arithmetic::Add:
    return arithmetic_publish_scalar(
        native_body, result,
        real ? LLVMBuildFAdd(
                   native_body.get_builder(), *left_value, *right_value, "")
             : LLVMBuildAdd(
                   native_body.get_builder(), *left_value, *right_value, ""));

  case Arithmetic::Subtract:
    return arithmetic_publish_scalar(
        native_body, result,
        real ? LLVMBuildFSub(
                   native_body.get_builder(), *left_value, *right_value, "")
             : LLVMBuildSub(
                   native_body.get_builder(), *left_value, *right_value, ""));

  case Arithmetic::Multiply:
    return arithmetic_publish_scalar(
        native_body, result,
        real ? LLVMBuildFMul(
                   native_body.get_builder(), *left_value, *right_value, "")
             : LLVMBuildMul(
                   native_body.get_builder(), *left_value, *right_value, ""));

  case Arithmetic::Divide:
    return arithmetic_publish_scalar(
        native_body, result,
        real ? LLVMBuildFDiv(
                   native_body.get_builder(), *left_value, *right_value, "")
        : signed_value
            ? LLVMBuildSDiv(
                  native_body.get_builder(), *left_value, *right_value, "")
            : LLVMBuildUDiv(
                  native_body.get_builder(), *left_value, *right_value, ""));

  case Arithmetic::Modulo:
    return arithmetic_publish_scalar(
        native_body, result,
        real ? LLVMBuildFRem(
                   native_body.get_builder(), *left_value, *right_value, "")
        : signed_value
            ? LLVMBuildSRem(
                  native_body.get_builder(), *left_value, *right_value, "")
            : LLVMBuildURem(
                  native_body.get_builder(), *left_value, *right_value, ""));
  }
}

auto Llvm::Emission::Computation::negate(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Pack& operand) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = arithmetic_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto value = arithmetic_find_scalar(native_body, operand);
  if (!value || !arithmetic_has_native_carrier(*carriers, carrier, *value)) {
    return False;
  }

  LLVMValueRef selected =
      carriers->is_real(carrier)
          ? LLVMBuildFNeg(native_body.get_builder(), *value, "")
          : LLVMBuildNeg(native_body.get_builder(), *value, "");
  return arithmetic_publish_scalar(native_body, result, selected);
}

static auto integer_max(Count bits, Bool signed_value) -> U64 {
  if (!signed_value) {
    return bits >= 64 ? U64(-1) : (U64(1) << bits) - 1;
  }
  return bits >= 64 ? U64(-1) >> 1 : (U64(1) << (bits - 1)) - 1;
}

static auto integer_min(Count bits) -> S64 {
  return bits >= 64 ? S64(U64(1) << 63) : -(S64(1) << (bits - 1));
}

static auto clamp_integer(
    LLVMBuilderRef builder,
    LLVMValueRef value,
    Bool source_signed,
    Count source_bits,
    Bool target_signed,
    Count target_bits) -> LLVMValueRef {
  LLVMTypeRef source_type = LLVMTypeOf(value);
  LLVMValueRef selected = value;
  if (source_signed && !target_signed) {
    LLVMValueRef negative = LLVMBuildICmp(
        builder, LLVMIntSLT, selected, LLVMConstInt(source_type, 0, 1), "");
    selected = LLVMBuildSelect(
        builder, negative, LLVMConstInt(source_type, 0, 0), selected, "");
  }

  U64 source_max = integer_max(source_bits, source_signed);
  U64 target_max = integer_max(target_bits, target_signed);
  if (target_max < source_max) {
    LLVMValueRef maximum = LLVMConstInt(source_type, target_max, 0);
    LLVMValueRef above = LLVMBuildICmp(
        builder, source_signed ? LLVMIntSGT : LLVMIntUGT, selected, maximum,
        "");
    selected = LLVMBuildSelect(builder, above, maximum, selected, "");
  }

  if (source_signed && target_signed && target_bits < source_bits) {
    S64 target_minimum = integer_min(target_bits);
    LLVMValueRef minimum = LLVMConstInt(source_type, U64(target_minimum), 1);
    LLVMValueRef below =
        LLVMBuildICmp(builder, LLVMIntSLT, selected, minimum, "");
    selected = LLVMBuildSelect(builder, below, minimum, selected, "");
  }
  return selected;
}

static auto convert_real_to_integer(
    LLVMBuilderRef builder,
    LLVMValueRef value,
    LLVMTypeRef target_type,
    Bool target_signed,
    Count target_bits) -> LLVMValueRef {
  LLVMTypeRef source_type = LLVMTypeOf(value);
  LLVMValueRef zero_real = LLVMConstReal(source_type, 0.0);
  LLVMValueRef unordered =
      LLVMBuildFCmp(builder, LLVMRealUNO, value, value, "");
  LLVMValueRef below = LLVMBuildFCmp(
      builder, LLVMRealOLE, value,
      LLVMConstReal(
          source_type,
          target_signed ? R64(integer_min(target_bits)) : R64(0.0)),
      "");
  LLVMValueRef above = LLVMBuildFCmp(
      builder, LLVMRealOGE, value,
      LLVMConstReal(source_type, R64(integer_max(target_bits, target_signed))),
      "");
  LLVMValueRef outside = LLVMBuildOr(
      builder, unordered, LLVMBuildOr(builder, below, above, ""), "");
  LLVMValueRef safe = LLVMBuildSelect(builder, outside, zero_real, value, "");
  LLVMValueRef converted =
      target_signed ? LLVMBuildFPToSI(builder, safe, target_type, "")
                    : LLVMBuildFPToUI(builder, safe, target_type, "");
  LLVMValueRef minimum = LLVMConstInt(
      target_type, target_signed ? U64(integer_min(target_bits)) : U64(0),
      bool(target_signed));
  LLVMValueRef maximum = LLVMConstInt(
      target_type, integer_max(target_bits, target_signed),
      bool(target_signed));
  LLVMValueRef bounded =
      LLVMBuildSelect(builder, above, maximum, converted, "");
  bounded = LLVMBuildSelect(builder, below, minimum, bounded, "");
  return LLVMBuildSelect(
      builder, unordered, LLVMConstInt(target_type, 0, 0), bounded, "");
}

auto Llvm::Emission::Computation::convert(
    const Tetrodotoxin::Source::Type& source_carrier,
    const Tetrodotoxin::Source::Type& target_carrier,
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Pack& source) const -> Bool {
  auto carriers = arithmetic_select_carriers(body);
  auto value = arithmetic_find_scalar(body, source);
  auto source_type = carriers ? carriers->get_type(source_carrier)
                              : Core::Option<LLVMTypeRef>();
  auto target_type = carriers ? carriers->get_type(target_carrier)
                              : Core::Option<LLVMTypeRef>();
  BAIL_IF(
      !carriers || !value || !source_type || !target_type ||
      LLVMTypeOf(*value) != *source_type);

  Bool source_real = carriers->is_real(source_carrier);
  Bool target_real = carriers->is_real(target_carrier);
  Bool source_signed = carriers->is_signed(source_carrier);
  Bool target_signed = carriers->is_signed(target_carrier);
  LLVMValueRef converted = nullptr;
  if (source_real && target_real) {
    converted = LLVMBuildFPCast(body.get_builder(), *value, *target_type, "");
  } else if (source_real) {
    converted = convert_real_to_integer(
        body.get_builder(), *value, *target_type, target_signed,
        LLVMGetIntTypeWidth(*target_type));
  } else if (target_real) {
    converted =
        source_signed
            ? LLVMBuildSIToFP(body.get_builder(), *value, *target_type, "")
            : LLVMBuildUIToFP(body.get_builder(), *value, *target_type, "");
  } else {
    LLVMValueRef clamped = clamp_integer(
        body.get_builder(), *value, source_signed,
        LLVMGetIntTypeWidth(*source_type), target_signed,
        LLVMGetIntTypeWidth(*target_type));
    converted = LLVMBuildIntCast2(
        body.get_builder(), clamped, *target_type, bool(source_signed), "");
  }
  return converted && arithmetic_publish_scalar(body, result, converted);
}

// Calls receive arguments in resolved parameter order. Call owns semantic
// fitting while Computation owns native parameter carriers and result
// transport.

static auto comparison_find_scalar(
    const Llvm::Module::Body& body,
    const Tetrodotoxin::Source::Pack& pack) -> Core::Option<LLVMValueRef> {
  auto values = body.find_values(pack);
  if (!values || values->get_size() != 1) {
    return {};
  }

  return values->get_data()[0];
}

static auto comparison_select_carriers(const Llvm::Module::Body& body)
    -> Core::Option<const Llvm::Module::Carriers&> {
  return body.get_program().get_carriers();
}

static auto emit_comparison(
    Llvm::Module::Body& body,
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Pack& left,
    const Tetrodotoxin::Source::Pack& right,
    LLVMRealPredicate real,
    LLVMIntPredicate signed_integer,
    LLVMIntPredicate unsigned_integer) -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = comparison_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto left_value = comparison_find_scalar(native_body, left);
  auto right_value = comparison_find_scalar(native_body, right);
  auto native = carriers->get_type(carrier);
  if (!left_value || !right_value || !native ||
      LLVMTypeOf(*left_value) != *native ||
      LLVMTypeOf(*right_value) != *native) {
    return False;
  }

  LLVMValueRef selected =
      carriers->is_real(carrier)
          ? LLVMBuildFCmp(
                native_body.get_builder(), real, *left_value, *right_value, "")
          : LLVMBuildICmp(
                native_body.get_builder(),
                carriers->is_signed(carrier) ? signed_integer
                                             : unsigned_integer,
                *left_value, *right_value, "");
  Core::Static::Vector<LLVMValueRef, 1> values = {{selected}};
  return native_body.publish_values(result, values.get_view());
}

auto Llvm::Emission::Computation::compare(
    Comparison operation,
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Pack& left,
    const Tetrodotoxin::Source::Pack& right) const -> Bool {
  switch (operation) {
  case Comparison::Equal:
    return emit_comparison(
        body, carrier, result, left, right, LLVMRealOEQ, LLVMIntEQ, LLVMIntEQ);

  case Comparison::NotEqual:
    return emit_comparison(
        body, carrier, result, left, right, LLVMRealUNE, LLVMIntNE, LLVMIntNE);

  case Comparison::Less:
    return emit_comparison(
        body, carrier, result, left, right, LLVMRealOLT, LLVMIntSLT,
        LLVMIntULT);

  case Comparison::LessEqual:
    return emit_comparison(
        body, carrier, result, left, right, LLVMRealOLE, LLVMIntSLE,
        LLVMIntULE);

  case Comparison::Greater:
    return emit_comparison(
        body, carrier, result, left, right, LLVMRealOGT, LLVMIntSGT,
        LLVMIntUGT);

  case Comparison::GreaterEqual:
    return emit_comparison(
        body, carrier, result, left, right, LLVMRealOGE, LLVMIntSGE,
        LLVMIntUGE);
  }
}

auto Llvm::Emission::Computation::compare_bytes(
    Comparison operation,
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Pack& left,
    const Tetrodotoxin::Source::Pack& right) const -> Bool {
  if (operation != Comparison::Equal && operation != Comparison::NotEqual) {
    return False;
  }

  auto left_value = comparison_find_scalar(body, left);
  auto right_value = comparison_find_scalar(body, right);
  if (!left_value || !right_value ||
      LLVMGetTypeKind(LLVMTypeOf(*left_value)) != LLVMStructTypeKind ||
      LLVMGetTypeKind(LLVMTypeOf(*right_value)) != LLVMStructTypeKind) {
    return False;
  }

  LLVMBuilderRef builder = body.get_builder();
  LLVMValueRef left_data = LLVMBuildExtractValue(builder, *left_value, 0, "");
  LLVMValueRef left_size = LLVMBuildExtractValue(builder, *left_value, 1, "");
  LLVMValueRef right_data = LLVMBuildExtractValue(builder, *right_value, 0, "");
  LLVMValueRef right_size = LLVMBuildExtractValue(builder, *right_value, 1, "");
  if (!left_data || !left_size || !right_data || !right_size) {
    return False;
  }

  LLVMValueRef function = body.get_function();
  LLVMContextRef context =
      LLVMGetModuleContext(&body.get_program().get_module());
  LLVMBasicBlockRef unequal = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef matching =
      LLVMAppendBasicBlockInContext(context, function, "bytes.matching");
  LLVMBasicBlockRef content =
      LLVMAppendBasicBlockInContext(context, function, "bytes.content");
  LLVMBasicBlockRef done =
      LLVMAppendBasicBlockInContext(context, function, "bytes.done");
  LLVMValueRef size_equal =
      LLVMBuildICmp(builder, LLVMIntEQ, left_size, right_size, "");
  LLVMBuildCondBr(builder, size_equal, matching, done);

  LLVMPositionBuilderAtEnd(builder, matching);
  LLVMValueRef zero_size = LLVMConstInt(LLVMTypeOf(left_size), 0, 0);
  LLVMValueRef empty =
      LLVMBuildICmp(builder, LLVMIntEQ, left_size, zero_size, "");
  LLVMBuildCondBr(builder, empty, done, content);

  LLVMPositionBuilderAtEnd(builder, content);
  LLVMModuleRef module = &body.get_program().get_module();
  LLVMValueRef compare_function = LLVMGetNamedFunction(module, "memcmp");
  LLVMTypeRef compare_parameters[] = {
    LLVMTypeOf(left_data), LLVMTypeOf(right_data), LLVMTypeOf(left_size)};
  LLVMTypeRef compare_type = LLVMFunctionType(
      LLVMInt32TypeInContext(context), compare_parameters, 3, 0);
  if (!compare_function) {
    compare_function = LLVMAddFunction(module, "memcmp", compare_type);
  }
  LLVMValueRef compare_arguments[] = {left_data, right_data, left_size};
  LLVMValueRef comparison = LLVMBuildCall2(
      builder, compare_type, compare_function, compare_arguments, 3, "");
  LLVMValueRef content_equal = LLVMBuildICmp(
      builder, LLVMIntEQ, comparison,
      LLVMConstInt(LLVMInt32TypeInContext(context), 0, 0), "");
  LLVMBuildBr(builder, done);

  LLVMPositionBuilderAtEnd(builder, done);
  LLVMValueRef equal =
      LLVMBuildPhi(builder, LLVMInt1TypeInContext(context), "");
  LLVMValueRef incoming_values[] = {
    LLVMConstInt(LLVMInt1TypeInContext(context), 0, 0),
    LLVMConstInt(LLVMInt1TypeInContext(context), 1, 0), content_equal};
  LLVMBasicBlockRef incoming_blocks[] = {unequal, matching, content};
  LLVMAddIncoming(equal, incoming_values, incoming_blocks, 3);
  LLVMValueRef selected =
      operation == Comparison::Equal ? equal : LLVMBuildNot(builder, equal, "");
  Core::Static::Vector<LLVMValueRef, 1> values = {{selected}};
  return body.publish_values(result, values.get_view());
}

// Construction assembles inline values or allocates Object payloads through
// the carrier selected by the exact source Type.
