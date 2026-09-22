// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/source/abstract.hpp"

namespace Puffer {

// Publisher commits one complete named Product Pack beneath a single output
// root. It owns filesystem validation and atomic replacement, never semantic
// traversal or product naming.
class Publisher {
 public:
  constexpr explicit Publisher(Perimortem::Core::View::Bytes root)
      : root(root) {}

  auto publish(const Tetrodotoxin::Source::Pack& products) const -> Bool;

 private:
  Perimortem::Core::View::Bytes root;
};

}  // namespace Puffer
