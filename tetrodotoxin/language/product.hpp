// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/source/constant.hpp"

namespace Tetrodotoxin::Language {

// Product is one immutable named byte result produced after graph completion.
// Its relative name is publication intent rather than semantic identity inside
// the source graph, and its bytes remain valid for the result Arena lifetime.
class Product : public Tetrodotoxin::Source::Constant {
 public:
  TTX_CONTRACT(Product, Tetrodotoxin::Source::Constant);

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes value) -> Product&;

  TTX_NAME(name);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_value() const -> Perimortem::Core::View::Bytes {
    return value;
  }

 private:
  constexpr Product(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes value)
      : name(name), value(value) {}

  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Bytes value;
};

}  // namespace Tetrodotoxin::Language
