// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// R64 is the standard 8 byte floating point representation.
// For ABI evaluation it can be used to represent C/C++'s `double`.
class R64 : public Model::Types::Real {
 public:
  TTX_NAME("R64"_view);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;
  constexpr auto get_width() const -> Count override { return 64; }
  constexpr auto get_size() const -> Count override { return sizeof(::R64); }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::R64);
  }

 private:
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "R64 is stored as an 8 byte IEEE floating value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
