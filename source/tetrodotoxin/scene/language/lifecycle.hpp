// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Scene::Language {

// Lifecycle names the five places where App can enter a live Scene instance.
// The value is only a relationship label. Each retained role points to the real
// Library Function that owns its signature, body, and compiled behavior.
enum class Lifecycle : U8 {
  Prepare,
  Pause,
  Resume,
  Update,
  Release,
};

}  // namespace Tetrodotoxin::Scene::Language
