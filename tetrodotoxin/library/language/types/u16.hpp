// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// U16 is the standard sixteen bit Type implementing the Unsigned
// domain.
class U16 : public Model::Types::Unsigned {
 public:
  TTX_NAME("U16"_view);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;
  constexpr auto get_width() const -> Count override { return 16; }
  constexpr auto get_size() const -> Count override { return sizeof(::U16); }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::U16);
  }

 private:
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "U16 is stored as a 2 byte unsigned integer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
