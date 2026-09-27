// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// The native bridge enters LLVM before Perimortem so LLVM's standard
// declarations remain confined to this implementation unit.
#if __has_include("llvm/IR/IRBuilder.h")
#include "llvm/IR/IRBuilder.h"
#else
#error LLVM IRBuilder is required by the LLVM Terminal
#endif

#include "perimortem/core/math.hpp"

#include "llvm-c/Core.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CBindingWrapping.h"
#include "tetrodotoxin/terminal/llvm/module/body.hpp"
#include "tetrodotoxin/terminal/llvm/module/c_abi.hpp"
#include "tetrodotoxin/terminal/llvm/module/program.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;

enum class CAbiClass : U8 {
  Empty,
  Integer,
  Sse,
};

static auto select_program(Llvm::Module::Emission& emission)
    -> Core::Option<Llvm::Module::Program&> {
  if (emission.get_kind() == Llvm::Module::Emission::Kind::Module) {
    return static_cast<Llvm::Module::Program&>(emission);
  }
  return static_cast<Llvm::Module::Body&>(emission).get_program();
}

static auto select_body(Llvm::Module::Emission& emission)
    -> Core::Option<Llvm::Module::Body&> {
  return emission.get_kind() == Llvm::Module::Emission::Kind::Body
             ? Core::Option<Llvm::Module::Body&>(
                   static_cast<Llvm::Module::Body&>(emission))
             : Core::Option<Llvm::Module::Body&>();
}

static auto merge_class(CAbiClass& target, CAbiClass incoming) -> void {
  if (target == CAbiClass::Integer || incoming == CAbiClass::Empty) {
    return;
  }
  if (incoming == CAbiClass::Integer || target == CAbiClass::Empty) {
    target = incoming;
  }
}

static auto mark_class(
    CAbiClass (&classes)[2],
    Count offset,
    Count size,
    CAbiClass incoming) -> Bool {
  if (size == 0 || offset >= 16 || size > 16 - offset) {
    return False;
  }

  Count first = offset / 8;
  Count last = (offset + size - 1) / 8;
  merge_class(classes[first], incoming);
  if (last != first) {
    merge_class(classes[last], incoming);
  }
  return True;
}

static auto classify(
    const llvm::DataLayout& layout,
    llvm::Type& type,
    Count offset,
    CAbiClass (&classes)[2]) -> Bool {
  Count size = layout.getTypeAllocSize(&type).getFixedValue();
  if (type.isIntegerTy() || type.isPointerTy()) {
    return mark_class(classes, offset, size, CAbiClass::Integer);
  }
  if (type.isFloatingPointTy()) {
    return Bool(size <= 8 && mark_class(classes, offset, size, CAbiClass::Sse));
  }
  if (auto* array = llvm::dyn_cast<llvm::ArrayType>(&type)) {
    llvm::Type& element = *array->getElementType();
    Count stride = layout.getTypeAllocSize(&element).getFixedValue();
    for (Count index = 0; index < array->getNumElements(); index++) {
      if (!classify(layout, element, offset + index * stride, classes)) {
        return False;
      }
    }
    return True;
  }
  if (auto* structure = llvm::dyn_cast<llvm::StructType>(&type)) {
    if (structure->isOpaque() || structure->isPacked()) {
      return False;
    }
    const llvm::StructLayout& structure_layout =
        *layout.getStructLayout(structure);
    for (Count index = 0; index < structure->getNumElements(); index++) {
      if (!classify(
              layout, *structure->getElementType(U32(index)),
              offset + structure_layout.getElementOffset(U32(index)),
              classes)) {
        return False;
      }
    }
    return True;
  }
  return False;
}

auto Llvm::Module::CAbi::select_direct_type(
    Llvm::Module::Emission& emission,
    LLVMTypeRef semantic) -> Core::Option<LLVMTypeRef> {
  auto program = select_program(emission);
  llvm::Type& type = *llvm::unwrap(semantic);
  if (!program) {
    return {};
  }
  if (!type.isAggregateType()) {
    return semantic;
  }

  llvm::Module& module = *llvm::unwrap(&program->get_module());
  const llvm::DataLayout& layout = module.getDataLayout();
  Count size = layout.getTypeAllocSize(&type).getFixedValue();
  if (size == 0 || size > 16) {
    return {};
  }

  CAbiClass classes[2] = {CAbiClass::Empty, CAbiClass::Empty};
  if (!classify(layout, type, 0, classes)) {
    return {};
  }

  llvm::LLVMContext& context = *llvm::unwrap(&program->get_context());
  Count count = size > 8 ? 2 : 1;
  llvm::Type* chunks[2];
  for (Count index = 0; index < count; index++) {
    Count chunk_size = Core::Math::min(Count(8), size - index * 8);
    if (classes[index] == CAbiClass::Sse) {
      chunks[index] =
          chunk_size <= 4
              ? static_cast<llvm::Type*>(llvm::Type::getFloatTy(context))
              : static_cast<llvm::Type*>(llvm::Type::getDoubleTy(context));
    } else {
      chunks[index] = llvm::IntegerType::get(context, U32(chunk_size * 8));
    }
  }

  return count == 1
             ? Core::Option<LLVMTypeRef>(llvm::wrap(chunks[0]))
             : Core::Option<LLVMTypeRef>(llvm::wrap(
                   llvm::StructType::get(
                       context, llvm::ArrayRef<llvm::Type*>(chunks, count))));
}

auto Llvm::Module::CAbi::convert(
    Llvm::Module::Emission& emission,
    LLVMTypeRef target,
    LLVMValueRef value,
    Core::View::Bytes name) -> Core::Option<LLVMValueRef> {
  auto body = select_body(emission);
  LLVMTypeRef source = LLVMTypeOf(value);
  if (!body) {
    return {};
  }
  if (source == target) {
    return value;
  }

  llvm::Module& module = *llvm::unwrap(&body->get_program().get_module());
  const llvm::DataLayout& layout = module.getDataLayout();
  llvm::Type& source_type = *llvm::unwrap(source);
  llvm::Type& target_type = *llvm::unwrap(target);
  Count source_size = layout.getTypeAllocSize(&source_type).getFixedValue();
  Count target_size = layout.getTypeAllocSize(&target_type).getFixedValue();
  llvm::Align source_alignment = layout.getABITypeAlign(&source_type);
  llvm::Align target_alignment = layout.getABITypeAlign(&target_type);
  llvm::Type& storage_type =
      source_size > target_size || (source_size == target_size &&
                                    source_alignment > target_alignment)
          ? source_type
          : target_type;
  llvm::Align alignment =
      source_alignment > target_alignment ? source_alignment : target_alignment;
  LLVMValueRef native_address =
      body->create_entry_alloca(llvm::wrap(&storage_type), name);
  if (!native_address) {
    return {};
  }

  llvm::IRBuilder<>& builder =
      *reinterpret_cast<llvm::IRBuilder<>*>(body->get_builder());
  llvm::Value& address = *llvm::unwrap(native_address);
  llvm::StoreInst& clear = *builder.CreateStore(
      llvm::Constant::getNullValue(&storage_type), &address);
  clear.setAlignment(alignment);
  llvm::StoreInst& store = *builder.CreateStore(llvm::unwrap(value), &address);
  store.setAlignment(alignment);
  llvm::LoadInst& loaded = *builder.CreateLoad(
      &target_type, &address,
      llvm::StringRef(
          reinterpret_cast<const char*>(name.get_data()), name.get_size()));
  loaded.setAlignment(alignment);
  return llvm::wrap(&loaded);
}
