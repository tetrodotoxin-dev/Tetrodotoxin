// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#if __has_include("llvm/IR/Module.h")
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#else
#error LLVM Module is required by Projection lowering
#endif

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/projection.hpp"
#include "tetrodotoxin/render/language/binding.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/projections.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

auto Llvm::Lowering::Projections::lower(
    Module::Program& program,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Projection> projections)
    -> Bool {
  llvm::Module& module = *llvm::unwrap(&program.get_module());
  llvm::LLVMContext& context = module.getContext();
  llvm::Type& pointer = *llvm::PointerType::getUnqual(context);
  llvm::IntegerType& count = *llvm::Type::getInt64Ty(context);
  Core::Static::Vector<llvm::Type*, 5> members = {
    {&pointer, &count, &count, &pointer, &count}};
  llvm::StructType& projection_type = *llvm::StructType::get(
      context,
      llvm::ArrayRef<llvm::Type*>(members.get_data(), members.get_size()));
  Core::Static::Vector<llvm::Type*, 2> resource_members = {{&count, &count}};
  llvm::StructType& resource_type = *llvm::StructType::get(
      context, llvm::ArrayRef<llvm::Type*>(
                   resource_members.get_data(), resource_members.get_size()));

  for (const Tetrodotoxin::Terminal::Abi::Projection& projection :
       projections) {
    const auto& shader = projection.get_program();
    const auto& instance = shader.get_instance();
    const auto& parameters = shader.get_instance_parameters_field();
    auto payload = program.get_carriers().get_payload(instance);
    auto field_index = program.get_carriers().get_field_index(parameters);
    auto type = parameters.get_type().select<Tetrodotoxin::Source::Type>();
    auto parameter_type = type ? program.get_carriers().get_type(*type)
                               : Core::Option<LLVMTypeRef>();
    BAIL_IF(!payload || !field_index || !parameter_type);

    auto& payload_type = *llvm::cast<llvm::StructType>(llvm::unwrap(*payload));
    const llvm::DataLayout& layout = module.getDataLayout();
    Count offset = layout.getStructLayout(&payload_type)
                       ->getElementOffset(U32(*field_index));
    Count size =
        layout.getTypeAllocSize(llvm::unwrap(*parameter_type)).getFixedValue();
    BAIL_IF(size == 0);

    Memory::Dynamic::Vector<llvm::Constant*> resources;
    for (const Shader::Language::Binding& binding : shader.get_bindings()) {
      if (binding.get_kind() != Render::Language::Binding::Kind::Resource) {
        continue;
      }
      U64 source =
          U64(Perimortem::Graphics::Projection::ResourceSource::HostTexture);
      Count resource_offset = 0;
      auto instance_field = binding.get_instance_field();
      if (instance_field) {
        auto resource_index =
            program.get_carriers().get_field_index(*instance_field);
        BAIL_IF(!resource_index);
        resource_offset = layout.getStructLayout(&payload_type)
                              ->getElementOffset(U32(*resource_index));
        source = U64(
            Perimortem::Graphics::Projection::ResourceSource::InstanceTexture);
      }
      Core::Static::Vector<llvm::Constant*, 2> resource_values = {{
        llvm::ConstantInt::get(&count, source),
        llvm::ConstantInt::get(&count, resource_offset),
      }};
      resources.insert(
          llvm::ConstantStruct::get(
              &resource_type,
              llvm::ArrayRef<llvm::Constant*>(
                  resource_values.get_data(), resource_values.get_size())));
    }
    llvm::Constant* resources_pointer =
        llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(&pointer));
    if (resources.get_size() != 0) {
      llvm::ArrayType& resources_type =
          *llvm::ArrayType::get(&resource_type, resources.get_size());
      llvm::Constant& resources_initializer = *llvm::ConstantArray::get(
          &resources_type, llvm::ArrayRef<llvm::Constant*>(
                               resources.get_data(), resources.get_size()));
      auto* resources_global = new llvm::GlobalVariable(
          module, &resources_type, true, llvm::GlobalValue::PrivateLinkage,
          &resources_initializer);
      resources_global->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
      resources_pointer = resources_global;
    }

    llvm::GlobalVariable* module_symbol =
        module.getNamedGlobal(llvm_text(projection.get_module_symbol()));
    if (module_symbol == nullptr) {
      module_symbol = new llvm::GlobalVariable(
          module, llvm::Type::getInt8Ty(context), true,
          llvm::GlobalValue::ExternalLinkage, nullptr,
          llvm_text(projection.get_module_symbol()));
    }
    BAIL_IF(module.getNamedGlobal(llvm_text(projection.get_symbol())));

    Core::Static::Vector<llvm::Constant*, 5> values = {{
      module_symbol,
      llvm::ConstantInt::get(&count, offset),
      llvm::ConstantInt::get(&count, size),
      resources_pointer,
      llvm::ConstantInt::get(&count, resources.get_size()),
    }};
    llvm::Constant& initializer = *llvm::ConstantStruct::get(
        &projection_type,
        llvm::ArrayRef<llvm::Constant*>(values.get_data(), values.get_size()));
    auto* emitted = new llvm::GlobalVariable(
        module, &projection_type, true, llvm::GlobalValue::ExternalLinkage,
        &initializer, llvm_text(projection.get_symbol()));
    emitted->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  }
  return True;
}
