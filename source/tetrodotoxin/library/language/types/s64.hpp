// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// S64 is the standard sixty four bit Signed Type.
class S64 : public Model::Types::Signed {
 public:
  TTX_NAME("S64"_view);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;
  constexpr auto get_width() const -> Count override { return 64; }
  constexpr auto get_size() const -> Count override { return sizeof(::S64); }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::S64);
  }

 private:
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "S64 is stored as an 8 byte two's-complement integer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
