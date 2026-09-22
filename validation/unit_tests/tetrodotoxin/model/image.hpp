// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/terminal/llvm/execution.hpp"

namespace Validation::ModelTests {

// The test uses the system linker as an independent consumer of the terminal's
// object file. This owner retains loaded code while its Query is in use, then
// unloads it before removing the temporary artifacts. It owns no source graph.
class Image {
 public:
  explicit Image(const Tetrodotoxin::Terminal::Llvm::Execution& artifact);
  ~Image();
  Image(const Image&) = delete;
  auto is_set() const -> Bool { return entry != nullptr; }
  auto get_query() const -> Ttx::Semantic::Negotiation::Query;
  auto get_bindings() const -> Count { return bindings; }

 private:
  const Tetrodotoxin::Terminal::Llvm::Execution& artifact;
  mutable Count bindings = 0;
  void* library = nullptr;
  void (*entry)(const void*, void*) = nullptr;
  char directory[64] = "/tmp/ttx-model-XXXXXX";
  char object[96] = {};
  char binary[96] = {};
};

}  // namespace Validation::ModelTests
