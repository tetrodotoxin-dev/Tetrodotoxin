// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// S16 is the standard sixteen bit Signed Type.
class S16 : public Model::Types::Signed {
 public:
  TTX_NAME("S16"_view);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;
  constexpr auto get_width() const -> Count override { return 16; }
  constexpr auto get_size() const -> Count override { return sizeof(::S16); }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::S16);
  }

 private:
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "S16 is stored as a 2 byte two's-complement integer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
