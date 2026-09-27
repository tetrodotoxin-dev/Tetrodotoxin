// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/content.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Source::Contents {

// Memory publishes bytes already retained by a source owner. The owner supplies
// their admitted contiguous U8 representation and keeps both it and the bytes
// immutable through every borrowed observation. This lets an editor reuse its
// existing storage without adding a copy or making allocation part of Content.
// Other providers can fulfill the same Content contract through Block access.
class Memory {
 public:
  constexpr Memory(
      Perimortem::Core::View::Bytes bytes,
      const Ttx::Data::Form::Representation& representation)
      : bytes(bytes), representation(representation) {}

  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }

  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status;

 private:
  Perimortem::Core::View::Bytes bytes;
  const Ttx::Data::Form::Representation& representation;
};

}  // namespace Tetrodotoxin::Source::Contents
