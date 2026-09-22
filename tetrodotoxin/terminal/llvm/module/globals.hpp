// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/terminal/llvm/module/emission.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/pack.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Module {

class Globals {
 public:
  auto reserve_static(
      Emission& program,
      const Tetrodotoxin::Source::Addressable& addressable) const
      -> Perimortem::Core::Option<Bool>;

  auto reserve_foreign(
      Emission& program,
      const Tetrodotoxin::Source::Addressable& addressable,
      Perimortem::Core::View::Bytes abi,
      Perimortem::Core::View::Bytes symbol,
      Bool writable) const -> Perimortem::Core::Option<Bool>;

  auto complete(Emission& program, const Tetrodotoxin::Source::Addressable& addressable)
      const -> Bool;

  auto begin_initializer(
      Emission& program,
      const Tetrodotoxin::Source::Addressable& addressable) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto end_initializer(
      Emission& body,
      const Tetrodotoxin::Source::Addressable& addressable,
      const Tetrodotoxin::Library::Language::Model::Pack& value) const -> Bool;

  auto find_address(const Tetrodotoxin::Source::Addressable& addressable) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto find_symbol(const Tetrodotoxin::Source::Addressable& addressable) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;

  auto permits_foreign_write(const Tetrodotoxin::Source::Addressable& addressable) const
      -> Bool;

  auto get_foreign_addressables() const -> Perimortem::Core::View::Vector<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable>>;

 private:
  class Record {
   public:
    enum class Property : U8 {
      Foreign = 1,
      Writable = 2,
      External = 4,
      Published = 8,
    };

    Record(
        Bool foreign,
        Perimortem::Core::View::Bytes abi = {},
        Perimortem::Core::View::Bytes symbol = {},
        Bool writable = True,
        Bool external = False,
        Bool published = False)
        : abi(abi),
          symbol(symbol),
          properties(
              U8(foreign ? Property::Foreign : Property{}) |
              U8(writable ? Property::Writable : Property{}) |
              U8(external ? Property::External : Property{}) |
              U8(published ? Property::Published : Property{})) {}

    constexpr auto has(Property property) const -> Bool {
      return Bool(properties & U8(property));
    }

    Perimortem::Core::View::Bytes abi;
    Perimortem::Core::View::Bytes symbol;
    U8 properties;
    Perimortem::Core::Option<LLVMValueRef> global;
    Perimortem::Core::Option<LLVMValueRef> initializer_function;
  };

  auto reserve(
      Emission& program,
      const Tetrodotoxin::Source::Addressable& addressable,
      Record record) const -> Perimortem::Core::Option<Bool>;

  mutable Perimortem::Memory::Dynamic::
      Map<const Tetrodotoxin::Source::Addressable*, Record>
          records;
  mutable Perimortem::Memory::Dynamic::Vector<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable>>
      foreign_addressables;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Module
