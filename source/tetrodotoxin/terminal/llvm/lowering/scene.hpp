// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/scene/language/emission.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/execution.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Lowering {

// Scene projects a Scene Emission carried by a Library Statement into the
// runtime event boundary. Library remains unaware of Signals, while the LLVM
// Terminal can compose the two completed Dialect graphs it was asked to emit.
class Scene {
 public:
  Scene() = delete;

  static auto lower(
      const Execution& execution,
      const Tetrodotoxin::Scene::Language::Emission& emission) -> Bool;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Lowering
