// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// U8 is the standard eight bit Type implementing the Unsigned domain.
// Its name and representation match the Perimortem primitive exactly.
class U8 : public Model::Types::Unsigned {
 public:
  TTX_NAME("U8"_view);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;
  constexpr auto get_width() const -> Count override { return 8; }
  constexpr auto get_size() const -> Count override { return sizeof(::U8); }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::U8);
  }

 private:
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "U8 is stored as a 1 byte unsigned integer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
