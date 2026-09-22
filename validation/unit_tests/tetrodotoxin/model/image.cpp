// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/tetrodotoxin/model/image.hpp"

#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace Validation::ModelTests;

Image::Image(const Tetrodotoxin::Terminal::Llvm::Execution& artifact)
    : artifact(artifact) {
  if (!mkdtemp(directory)) {
    return;
  }

  snprintf(object, sizeof(object), "%s/function.o", directory);
  snprintf(binary, sizeof(binary), "%s/function.so", directory);
  const int output = open(object, O_CREAT | O_WRONLY | O_EXCL, 0600);
  if (output < 0) {
    return;
  }

  const auto bytes = artifact.get_object();
  Count offset = 0;
  while (offset < bytes.get_size()) {
    const auto written =
        write(output, bytes.get_data() + offset, bytes.get_size() - offset);
    if (written <= 0) {
      close(output);
      return;
    }

    offset += written;
  }

  close(output);

  const auto child = fork();
  if (child == 0) {
    execlp(
        "cc", "cc", "-shared", "-o", binary, object,
        static_cast<char*>(nullptr));
    _exit(127);
  }

  int status = 0;
  if (child < 0 || waitpid(child, &status, 0) != child || !WIFEXITED(status) ||
      WEXITSTATUS(status)) {
    return;
  }

  library = dlopen(binary, RTLD_NOW | RTLD_LOCAL);
  if (library) {
    entry = reinterpret_cast<void (*)(const void*, void*)>(
        dlsym(library, "ttx_entry"));
  }
}

Image::~Image() {
  if (library) {
    dlclose(library);
  }

  if (object[0]) {
    unlink(object);
  }

  if (binary[0]) {
    unlink(binary);
  }

  rmdir(directory);
}

auto Image::get_query() const -> Ttx::Semantic::Negotiation::Query {
  return Ttx::Semantic::Negotiation::Query(
      {this,
       [](const void* self, perimortem_uuid id,
          ttx_storage destination) -> ttx_binding_status {
         const auto& image = *static_cast<const Image*>(self);
         ++image.bindings;
         if (Perimortem::System::Uuid(id) != image.artifact.get_operation()) {
           return TTX_BINDING_UNSUPPORTED;
         }

         const ttx_invocation api = {
           self, &image.artifact.get_inputs(), &image.artifact.get_outputs(),
           [](const void* receiver, const void* input,
              void* output) -> ttx_data_status {
             static_cast<const Image*>(receiver)->entry(input, output);
             return TTX_DATA_SUCCESS;
           }};
         return ttx_binding_provide(
             ttx_invocation_representation(), &api, destination);
       },
       [](const void* self, perimortem_uuid id) -> ttx_binding_status {
         return Perimortem::System::Uuid(id) == static_cast<const Image*>(self)
                                                    ->artifact.get_operation()
                    ? TTX_BINDING_SATISFIED
                    : TTX_BINDING_UNSUPPORTED;
       }});
}
