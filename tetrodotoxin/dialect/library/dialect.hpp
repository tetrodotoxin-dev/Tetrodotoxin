// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/source/dialect.hpp"

namespace Tetrodotoxin::Dialect::Library {

// This Library provider consumes one authored Execution function from the
// supplied Cursor using the TTX Code vocabulary and leaves it at the next
// production. Its context supplies Type names, while the host assigns the
// operation identity expected by callers of the emitted function. The returned
// Publication owns this invocation's Monograph independently of the Cursor.
// An enclosing dialect can retain or forward that result before a terminal
// consumes its model; invoking Library does not create a new source root.
class Dialect {
 public:
  explicit constexpr Dialect(Perimortem::System::Uuid operation)
      : operation(operation) {}
  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage output) const
      -> Ttx::Semantic::Negotiation::Binding::Status;

 private:
  Perimortem::System::Uuid operation;
};

}  // namespace Tetrodotoxin::Dialect::Library
