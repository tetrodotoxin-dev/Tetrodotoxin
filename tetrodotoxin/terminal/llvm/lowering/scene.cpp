// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#if __has_include("llvm/IR/IRBuilder.h")
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#else
#error LLVM IRBuilder is required by the Scene native Terminal
#endif

#include "tetrodotoxin/terminal/abi/symbol.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/scene.hpp"
#include "tetrodotoxin/source/addressable.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

auto Llvm::Lowering::Scene::lower(
    const Execution& execution,
    const Tetrodotoxin::Scene::Language::Emission& emission) -> Bool {
  BAIL_IF(emission.get_payload());
  auto signal = emission.get_signal();
  BAIL_IF(!signal);

  Module::Body& body = execution.get_body();
  auto callable = body.get_callable();
  BAIL_IF(!callable || callable->get_parameters().get_size() == 0);
  auto parameter = callable->get_parameters().get_abstract(0);
  auto self = parameter ? parameter->select<Tetrodotoxin::Source::Addressable>()
                        : Core::Option<const Tetrodotoxin::Source::Addressable&>();
  auto address = self ? body.find_address(*self) : Core::Option<LLVMValueRef>();
  auto type = self ? self->get_type().select<Tetrodotoxin::Source::Type>()
                   : Core::Option<const Tetrodotoxin::Source::Type&>();
  auto native = type ? execution.get_program().get_carriers().get_type(*type)
                     : Core::Option<LLVMTypeRef>();
  BAIL_IF(!address || !type || !native);

  auto& module = *llvm::unwrap(&execution.get_program().get_module());
  llvm::IRBuilder<>& builder =
      *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
  llvm::Value& object =
      *builder.CreateLoad(llvm::unwrap(*native), llvm::unwrap(*address));

  Tetrodotoxin::Terminal::Abi::Symbol symbol(
      execution.get_program().get_arena(), *signal,
      Tetrodotoxin::Terminal::Abi::Symbol::Kind::ReadOnly,
      execution.get_program().get_unit());
  llvm::GlobalVariable* token =
      module.getNamedGlobal(llvm_text(symbol.get_view()));
  if (!token) {
    llvm::Type& byte = *llvm::Type::getInt8Ty(module.getContext());
    token = new llvm::GlobalVariable(
        module, &byte, true, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(&byte, 0), llvm_text(symbol.get_view()));
  }

  llvm::Type& pointer = *llvm::PointerType::getUnqual(module.getContext());
  llvm::FunctionType& signature = *llvm::FunctionType::get(
      llvm::Type::getVoidTy(module.getContext()), {&pointer, &pointer}, false);
  builder.CreateCall(
      module.getOrInsertFunction("tetrodotoxin_scene_emit", &signature),
      {&object, token});
  return True;
}
