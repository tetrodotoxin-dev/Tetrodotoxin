// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

namespace Tetrodotoxin::Linker::Elf {

// Object packages data that should travel with one Linux native product. Each
// named range becomes a normal global ELF symbol, which lets generated code
// borrow the bytes directly after the application link without opening a
// second deployment file. Symbol names remain borrowed until build returns,
// while the object copies every data range as it is added.
class Object {
 public:
  auto add_read_only(
      Perimortem::Core::View::Bytes symbol,
      Perimortem::Core::View::Bytes end_symbol,
      Perimortem::Core::View::Bytes contents) -> Bool;

  auto build() const
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;

 private:
  class Symbol {
   public:
    constexpr Symbol(
        Perimortem::Core::View::Bytes name,
        Count offset,
        Count size)
        : name(name), offset(offset), size(size) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }
    constexpr auto get_offset() const -> Count { return offset; }
    constexpr auto get_size() const -> Count { return size; }

   private:
    Perimortem::Core::View::Bytes name;
    Count offset;
    Count size;
  };

  Perimortem::Memory::Dynamic::Bytes read_only;
  Perimortem::Memory::Dynamic::Vector<Symbol> symbols;
};

}  // namespace Tetrodotoxin::Linker::Elf
