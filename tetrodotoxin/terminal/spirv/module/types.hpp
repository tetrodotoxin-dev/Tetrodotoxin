// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/terminal/spirv/assembler/spir_v.hpp"
#include "tetrodotoxin/terminal/spirv/module/ids.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Terminal::Spirv::Module {

// Types derives one request local SPIR V type table from exact Library Types.
// Entries retain semantic identities only as keys while every emitted id and
// storage class remains physical module state.
class Types {
 public:
  explicit Types(Ids& ids);

  auto collect(const Tetrodotoxin::Library::Language::Model::Type& type)
      -> Bool;
  auto collect_pointer(
      const Tetrodotoxin::Library::Language::Model::Type& type,
      Assembler::SpirV::StorageClass storage) -> Bool;
  auto collect_resource(
      const Tetrodotoxin::Library::Language::Model::Type& type) -> Bool;
  auto emit(Assembler::SpirV& assembler) const -> Bool;
  auto decorate_push(
      Assembler::SpirV& assembler,
      const Tetrodotoxin::Library::Language::Model::Type& type) const -> Bool;

  auto get_id(const Tetrodotoxin::Library::Language::Model::Type& type) const
      -> Perimortem::Core::Option<U32>;
  auto get_pointer_id(
      const Tetrodotoxin::Library::Language::Model::Type& type,
      Assembler::SpirV::StorageClass storage) const
      -> Perimortem::Core::Option<U32>;
  auto get_resource_pointer_id(
      const Tetrodotoxin::Library::Language::Model::Type& type) const
      -> Perimortem::Core::Option<U32>;
  auto get_unsigned_32_id() const -> Perimortem::Core::Option<U32>;

  constexpr auto get_void_id() const -> U32 { return void_id; }
  constexpr auto get_function_id() const -> U32 { return function_id; }

  auto requires_float64() const -> Bool;

  static auto select(const Tetrodotoxin::Source::Abstract& semantic)
      -> Perimortem::Core::Option<
          const Tetrodotoxin::Library::Language::Model::Type&>;

 private:
  class Entry {
   public:
    constexpr Entry(
        const Tetrodotoxin::Library::Language::Model::Type& type,
        U32 id)
        : type(type), id(id) {}

    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Library::Language::Model::Type>
        type;
    U32 id;
  };

  class Pointer {
   public:
    constexpr Pointer(
        const Tetrodotoxin::Library::Language::Model::Type& type,
        Assembler::SpirV::StorageClass storage,
        U32 id)
        : type(type), storage(storage), id(id) {}

    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Library::Language::Model::Type>
        type;
    Assembler::SpirV::StorageClass storage;
    U32 id;
  };

  class Resource {
   public:
    constexpr Resource(
        const Tetrodotoxin::Library::Language::Model::Type& type,
        const Tetrodotoxin::Library::Language::Model::Type& sampled,
        U32 image_id,
        U32 sampled_id,
        U32 pointer_id)
        : type(type),
          sampled(sampled),
          image_id(image_id),
          sampled_id(sampled_id),
          pointer_id(pointer_id) {}

    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Library::Language::Model::Type>
        type;
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Library::Language::Model::Type>
        sampled;
    U32 image_id;
    U32 sampled_id;
    U32 pointer_id;
  };

  auto find_resource(const Tetrodotoxin::Library::Language::Model::Type& type)
      const -> Perimortem::Core::Option<const Resource&>;

  Ids& ids;
  U32 void_id;
  U32 function_id;
  Perimortem::Memory::Dynamic::Vector<
      const Tetrodotoxin::Library::Language::Model::Type*>
      visiting;
  Perimortem::Memory::Dynamic::Vector<Entry> entries;
  Perimortem::Memory::Dynamic::Vector<Pointer> pointers;
  Perimortem::Memory::Dynamic::Vector<Resource> resources;
};

}  // namespace Tetrodotoxin::Terminal::Spirv::Module
