// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Ttx::Concept {

// Discovery lends the caller's receiver to the provider for one enumeration.
// The provider chooses which named values to expose and can produce each one
// directly from its own storage. Native references and bound views use the
// same receiver shape without forcing either representation onto the other.
//
// The receiver is borrowed, so the provider must finish calling it before
// enumeration returns. It cannot retain the Visitor for deferred work. Each
// discovery contract supplies the lifetime of the names and values it emits.
template <typename Value>
class Visitor {
 public:
  constexpr Visitor(
      void* source,
      void (*receive)(void*, Perimortem::Core::View::Bytes, Value))
      : source(source), receive(receive) {}

  template <typename Receiver>
    requires(!__is_same(__remove_cvref(Receiver), Visitor))
  constexpr explicit Visitor(Receiver& receiver)
      : Visitor(
            &receiver,
            [](void* source, Perimortem::Core::View::Bytes name, Value value) {
              (*static_cast<Receiver*>(source))(name, value);
            }) {}

  auto operator()(Perimortem::Core::View::Bytes name, Value value) const
      -> void {
    receive(source, name, value);
  }

 private:
  void* source;
  void (*receive)(void*, Perimortem::Core::View::Bytes, Value);
};

}  // namespace Ttx::Concept
