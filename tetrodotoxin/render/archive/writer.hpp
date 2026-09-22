// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/type_reference.hpp"
#include "tetrodotoxin/render/archive/tag.hpp"
#include "tetrodotoxin/render/language/layout.hpp"
#include "tetrodotoxin/render/language/monograph.hpp"
#include "tetrodotoxin/render/language/structure.hpp"

namespace Tetrodotoxin::Render::Archive {

// Writer records the Render facts another Workspace can query. Its framing is
// private to Render, which leaves SPIR V storage and instruction choices with
// the independent Terminal that consumes the reconstructed graph.
class Writer {
 public:
  static auto encode(const Tetrodotoxin::Render::Language::Monograph& monograph)
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
  auto write(const Tetrodotoxin::Language::TypeReference& reference) -> Bool;
  auto write(const Tetrodotoxin::Render::Language::Layout& layout) -> Bool;
  auto write(const Tetrodotoxin::Source::Abstract& declaration) -> Bool;
  auto write(const Tetrodotoxin::Render::Language::Structure& structure)
      -> Bool;
  auto write(const Tetrodotoxin::Render::Language::Monograph& monograph)
      -> Bool;

  Perimortem::Memory::Dynamic::Bytes bytes;
};

}  // namespace Tetrodotoxin::Render::Archive
