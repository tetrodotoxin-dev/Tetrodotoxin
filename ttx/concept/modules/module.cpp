// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/modules/module.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"

#include "perimortem/system/library.hpp"

using namespace Ttx;
using namespace Perimortem;

// Only the native loader needs to know that this module is an OS library. The
// public resource operations also admit embedded and managed implementations.
struct NativeModule {
  NativeModule(System::Library&& library, ttx_module_entry entry)
      : library(Core::Data::take(library)), entry(entry) {}
  System::Library library;
  ttx_module_entry entry;
};

static auto publish(System::Library&& library, void* symbol)
    -> Concept::Modules::Module {
  static const Core::Object<>::Descriptor descriptor(
      sizeof(NativeModule), alignof(NativeModule), [](U8* source) {
        reinterpret_cast<NativeModule*>(source)->~NativeModule();
      });
  auto storage = Core::Object<>::create(descriptor).get_payload();
  auto* module = new (storage, Core::Placement::Construct) NativeModule(
      Core::Data::take(library), reinterpret_cast<ttx_module_entry>(symbol));
  return Concept::Modules::Module(
      {module,
       [](const void* source) {
         Core::Object<>(reinterpret_cast<U8*>(const_cast<void*>(source)))
             .retain();
       },
       [](const void* source) {
         Core::Object<>(reinterpret_cast<U8*>(const_cast<void*>(source)))
             .release();
       },
       [](const void* source, ttx_semantic_query host,
          ttx_module_acquisition* output) {
         return static_cast<const NativeModule*>(source)->entry(host, output);
       }});
}

auto Concept::Modules::Module::load(
    Core::View::Bytes path,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<Module, Core::View::Bytes> {
  using Result = Utility::Result<Module, Core::View::Bytes>;
  return System::Library::open(path, errors)
      .visit(
          [&](System::Library& library) -> Result {
            return library
                .symbol(Core::NullTerminated::to_view(TTX_MODULE_ENTRY), errors)
                .visit(
                    [&](void* entry) -> Result {
                      return publish(Core::Data::take(library), entry);
                    },
                    [](Core::View::Bytes error) -> Result { return error; });
          },
          [](Core::View::Bytes error) -> Result { return error; });
}

auto Concept::Modules::Module::open(Ttx::Semantic::Negotiation::Query host)
    const -> Utility::Result<Acquisition, Data::Status> {
  ttx_module_acquisition output = {};
  auto status = value.open(value.source, host, &output);
  if (status != TTX_DATA_SUCCESS) {
    return static_cast<Data::Status>(status);
  }

  if (!output.root.operations || !output.root.operations->bind ||
      !output.release) {
    if (output.release) {
      output.release(output.owner);
    }
    return Data::Status::Invalid;
  }
  return Acquisition(output);
}
