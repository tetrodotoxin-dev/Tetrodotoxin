// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/shader/archive/tag.hpp"
#include "tetrodotoxin/shader/language/bridge.hpp"
#include "tetrodotoxin/shader/language/monograph.hpp"
#include "tetrodotoxin/shader/language/program.hpp"

namespace Tetrodotoxin::Shader::Archive {

// Writer keeps Shader relationships beside an opaque Library member payload.
// The child schema remains Library owned while Shader records the Render route,
// storage roles, and Bridge policy that give those executable identities GPU
// meaning.
class Writer {
 public:
  static auto encode(const Tetrodotoxin::Shader::Language::Monograph& monograph)
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;

 private:
  class Record {
   public:
    constexpr explicit Record(Count offset) : offset(offset) {}

    constexpr auto get_offset() const -> Count { return offset; }

   private:
    Count offset;
  };

  Writer();

  auto begin(Tag tag) -> Record;
  auto finish(Record record) -> Bool;
  auto write(U8 value) -> void;
  auto write(U32 value) -> void;
  auto write(U64 value) -> void;
  auto write(S64 value) -> void;
  auto write(R64 value) -> void;
  auto write(Perimortem::Core::View::Bytes value) -> Bool;
  auto write(const Tetrodotoxin::Source::Documentation& value) -> Bool;
  auto write(
      Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
          attributes) -> Bool;
  auto write(const Tetrodotoxin::Language::Definition& definition) -> Bool;
  auto write(const Tetrodotoxin::Shader::Language::Program& program) -> Bool;
  auto write(const Tetrodotoxin::Shader::Language::Bridge& bridge) -> Bool;

  Perimortem::Memory::Dynamic::Bytes bytes;
};

}  // namespace Tetrodotoxin::Shader::Archive
