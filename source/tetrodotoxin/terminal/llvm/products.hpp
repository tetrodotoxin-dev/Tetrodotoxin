// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/linker/fingerprint.hpp"
#include "tetrodotoxin/linker/import.hpp"

namespace Tetrodotoxin::Terminal::Llvm {

// Products keeps each completed LLVM view in the caller Arena. LLVM IR remains
// available for review, while the native object, target fingerprint, and import
// agreement travel to the build system that requested them.
class Products {
 public:
  constexpr Products(
      Perimortem::Core::View::Bytes llvm_ir,
      Perimortem::Core::View::Bytes object,
      Tetrodotoxin::Linker::Fingerprint abi_fingerprint = {},
      Perimortem::Core::View::Vector<Tetrodotoxin::Linker::Import> imports = {})
      : llvm_ir(llvm_ir),
        object(object),
        abi_fingerprint(abi_fingerprint),
        imports(imports) {}

  constexpr auto get_llvm_ir() const -> Perimortem::Core::View::Bytes {
    return llvm_ir;
  }

  constexpr auto get_object() const -> Perimortem::Core::View::Bytes {
    return object;
  }

  constexpr auto get_abi_fingerprint() const
      -> Tetrodotoxin::Linker::Fingerprint {
    return abi_fingerprint;
  }

  constexpr auto get_imports() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Linker::Import> {
    return imports;
  }

 private:
  Perimortem::Core::View::Bytes llvm_ir;
  Perimortem::Core::View::Bytes object;
  Tetrodotoxin::Linker::Fingerprint abi_fingerprint;
  Perimortem::Core::View::Vector<Tetrodotoxin::Linker::Import> imports;
};

}  // namespace Tetrodotoxin::Terminal::Llvm
