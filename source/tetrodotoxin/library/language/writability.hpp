// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Library::Language {

// Writability is the Library storage and evaluation policy retained by Fields
// and Locals that create an Addressable. Field visibility independently
// decides which selected callers may write that storage. Constant marks
// compile time value identity rather than an initialization time write window.
enum class Writability : ::U8 {
  Full,
  Internal,
  Constant,
};

}  // namespace Tetrodotoxin::Library::Language
