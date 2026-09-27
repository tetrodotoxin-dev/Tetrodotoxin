// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/llvm/module/literals.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "llvm-c/Core.h"

using namespace Perimortem;
using namespace Tetrodotoxin;

auto Terminal::Llvm::Module::Literals::create_bytes_view(
    Program& program,
    LLVMTypeRef type,
    Core::View::Bytes value,
    Core::Option<const Language::Resource&> resource)
    -> Core::Option<LLVMValueRef> {
  if (LLVMGetTypeKind(type) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(type) != 2) {
    return {};
  }

  LLVMContextRef context = &program.get_context();
  LLVMModuleRef module = &program.get_module();
  LLVMValueRef data = LLVMConstNull(LLVMPointerTypeInContext(context, 0));
  if (resource) {
    auto symbol = program.get_unit().find(*resource);
    BAIL_IF(!symbol);
    LLVMTypeRef contents_type =
        LLVMArrayType2(LLVMInt8TypeInContext(context), value.get_size());
    Memory::Managed::Bytes native_symbol(program.get_arena(), *symbol);
    native_symbol.append(0);
    LLVMValueRef global = LLVMAddGlobal(
        module, contents_type,
        reinterpret_cast<const char*>(native_symbol.get_data()));
    LLVMSetGlobalConstant(global, 1);
    LLVMSetLinkage(global, LLVMExternalLinkage);
    data = global;
  } else if (!value.is_empty()) {
    LLVMValueRef contents = LLVMConstStringInContext2(
        context, reinterpret_cast<const char*>(value.get_data()),
        value.get_size(), 1);
    LLVMValueRef global =
        LLVMAddGlobal(module, LLVMTypeOf(contents), "__ttx_bytes");
    LLVMSetGlobalConstant(global, 1);
    LLVMSetInitializer(global, contents);
    LLVMSetLinkage(global, LLVMPrivateLinkage);
    LLVMSetUnnamedAddress(global, LLVMGlobalUnnamedAddr);
    data = global;
  }

  LLVMValueRef count =
      LLVMConstInt(LLVMInt64TypeInContext(context), value.get_size(), 0);
  Core::Static::Vector<LLVMValueRef, 2> elements = {{data, count}};
  return LLVMConstNamedStruct(
      type, elements.get_data(), U32(elements.get_size()));
}
