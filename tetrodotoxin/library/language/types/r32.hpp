// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// R32 is the standard 4 byte floating point representation.
// For ABI evaluation it can be used to represent C/C++'s `float`.
class R32 : public Model::Types::Real {
 public:
  TTX_NAME("R32"_view);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;
  constexpr auto get_width() const -> Count override { return 32; }
  constexpr auto get_size() const -> Count override { return sizeof(::R32); }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::R32);
  }

 private:
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "R32 is stored as a 4 byte IEEE floating value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
