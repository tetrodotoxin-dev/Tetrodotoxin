// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/library.hpp"

#include <dlfcn.h>

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

using namespace Perimortem;

System::Library::Library(Library&& source) : handle(source.handle) {
  source.handle = nullptr;
}

System::Library::~Library() {
  if (handle && dlclose(handle) != 0) {
    Core::Diagnostics::Log::fatal(Core::NullTerminated::to_view(dlerror()));
  }
}

auto System::Library::open(
    Core::View::Bytes path,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<Library, Core::View::Bytes> {
  Memory::Dynamic::Bytes name(path);
  name.append(0);
  auto* handle = dlopen(
      reinterpret_cast<const char*>(name.get_view().get_data()),
      RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    return errors.proxy(Core::NullTerminated::to_view(dlerror()));
  }

  return Library(handle);
}

auto System::Library::symbol(
    Core::View::Bytes name,
    Memory::Allocator::Arena& errors) const
    -> Utility::Result<void*, Core::View::Bytes> {
  Memory::Dynamic::Bytes terminated(name);
  terminated.append(0);
  dlerror();
  auto* address = dlsym(
      handle, reinterpret_cast<const char*>(terminated.get_view().get_data()));
  if (const auto* error = dlerror()) {
    return errors.proxy(Core::NullTerminated::to_view(error));
  }

  return address;
}
