// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#if __has_include("llvm/IR/IRBuilder.h")
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#else
#error LLVM IRBuilder is required by the Graphics native Terminal
#endif

#include "tetrodotoxin/terminal/abi/symbol.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/graphics.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

auto Llvm::Lowering::Graphics::lower(
    Module::Program& program,
    const Tetrodotoxin::Terminal::Graphics::Products& products) -> Bool {
  const auto& instance = products.get_scene().get_instance();
  auto payload_type = program.get_carriers().get_payload(instance);
  BAIL_IF(!payload_type);

  llvm::Module& module = *llvm::unwrap(&program.get_module());
  llvm::LLVMContext& context = module.getContext();
  llvm::Type& pointer = *llvm::PointerType::getUnqual(context);
  llvm::Type& count = *llvm::Type::getInt64Ty(context);
  llvm::FunctionType& signature =
      *llvm::FunctionType::get(&count, {&pointer, &count, &pointer}, false);
  Tetrodotoxin::Terminal::Abi::Symbol symbol(
      program.get_arena(), instance,
      Tetrodotoxin::Terminal::Abi::Symbol::Kind::GraphicsChildren,
      program.get_unit());
  BAIL_IF(module.getNamedValue(llvm_text(symbol.get_view())));

  llvm::Function& function = *llvm::Function::Create(
      &signature, llvm::GlobalValue::ExternalLinkage,
      llvm_text(symbol.get_view()), module);
  llvm::BasicBlock& entry =
      *llvm::BasicBlock::Create(context, "entry", &function);
  llvm::BasicBlock& missing =
      *llvm::BasicBlock::Create(context, "missing", &function);
  llvm::IRBuilder<> builder(&entry);
  llvm::Value& object = *function.getArg(0);
  llvm::Value& selected_index = *function.getArg(1);
  llvm::Value& output = *function.getArg(2);

  auto hosted = products.get_hosted();
  for (Count index = 0; index < hosted.get_size(); index++) {
    const auto& selected = hosted.get_data()[index];
    auto field_index =
        program.get_carriers().get_field_index(selected.get_field());
    auto type = selected.get_field().get_type().select<Tetrodotoxin::Source::Type>();
    auto field_type = type ? program.get_carriers().get_type(*type)
                           : Core::Option<LLVMTypeRef>();
    BAIL_IF(!field_index || !type || !field_type);

    llvm::BasicBlock& match =
        *llvm::BasicBlock::Create(context, "child", &function);
    llvm::BasicBlock& next =
        index + 1 == hosted.get_size()
            ? missing
            : *llvm::BasicBlock::Create(context, "next", &function);
    llvm::Value& equal = *builder.CreateICmpEQ(
        &selected_index, llvm::ConstantInt::get(&count, index));
    builder.CreateCondBr(&equal, &match, &next);

    builder.SetInsertPoint(&match);
    llvm::Value* address = builder.CreateStructGEP(
        llvm::unwrap(*payload_type), &object, U32(*field_index));
    llvm::Type* child_type = llvm::unwrap(*field_type);
    auto element_index = selected.get_element_index();
    if (element_index) {
      auto* array = llvm::dyn_cast<llvm::ArrayType>(child_type);
      BAIL_IF(array == nullptr || *element_index >= array->getNumElements());
      address = builder.CreateInBoundsGEP(
          array, address,
          {builder.getInt64(0), builder.getInt64(*element_index)});
      child_type = array->getElementType();
    }

    llvm::Value* child = builder.CreateLoad(child_type, address);
    builder.CreateStore(child, &output);
    builder.CreateRet(
        llvm::ConstantInt::get(&count, selected.get_type_index()));

    if (&next != &missing) {
      builder.SetInsertPoint(&next);
    }
  }
  if (hosted.is_empty()) {
    builder.CreateBr(&missing);
  }

  builder.SetInsertPoint(&missing);
  builder.CreateStore(
      llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(context)),
      &output);
  builder.CreateRet(llvm::ConstantInt::get(&count, U64(-1)));
  return True;
}
