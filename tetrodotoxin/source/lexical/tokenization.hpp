// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/tokenization.hpp"

namespace Tetrodotoxin::Source::Lexical {

// This provider selects the TTX lexical vocabulary for Source's generic
// Tokenization operation. The returned Publication owns the classified input
// and retains the data access needed for token spelling. A native source
// can lend its bytes directly, while a remote source can populate local storage
// once. Both expose the same Cursor and Content contracts to a dialect.
class Tokenization {
 public:
  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }

  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
};

}  // namespace Tetrodotoxin::Source::Lexical
