// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// S8 is the standard eight bit Signed Type.
class S8 : public Model::Types::Signed {
 public:
  TTX_NAME("S8"_view);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;
  constexpr auto get_width() const -> Count override { return 8; }
  constexpr auto get_size() const -> Count override { return sizeof(::S8); }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::S8);
  }

 private:
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "S8 is stored as a 1 byte two's-complement integer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
