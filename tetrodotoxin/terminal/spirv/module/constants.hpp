// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/terminal/spirv/assembler/spir_v.hpp"
#include "tetrodotoxin/terminal/spirv/module/ids.hpp"
#include "tetrodotoxin/terminal/spirv/module/types.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Terminal::Spirv::Module {

// Constants assigns module ids to the exact folded values already owned by
// Library. It derives their wire representation without changing or replacing
// the semantic Constant that supplied the value.
class Constants {
 public:
  Constants(Ids& ids, Types& types) : ids(ids), types(types) {}

  auto collect(const Tetrodotoxin::Library::Language::Constant& constant)
      -> Bool;
  auto emit(Assembler::SpirV& assembler) const -> Bool;
  auto get_id(const Tetrodotoxin::Library::Language::Constant& constant) const
      -> Perimortem::Core::Option<U32>;

 private:
  class Entry {
   public:
    constexpr Entry(
        const Tetrodotoxin::Library::Language::Constant& constant,
        U32 id)
        : constant(constant), id(id) {}

    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Library::Language::Constant>
        constant;
    U32 id;
  };

  Ids& ids;
  Types& types;
  Perimortem::Memory::Dynamic::Vector<Entry> entries;
};

}  // namespace Tetrodotoxin::Terminal::Spirv::Module
