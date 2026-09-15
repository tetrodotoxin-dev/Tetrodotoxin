// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/abstract.hpp"

namespace Validation::ConceptTests {

// Both the executable and its loaded fixture instantiate this exact C++ type.
// Matching names and layout still do not authorize native recovery across the
// module boundary. The portable operations remain fully usable there.
struct Subject {
  auto get_data() const -> Perimortem::Core::View::Bytes {
    return "subject"_view;
  }
  auto satisfies(const Ttx::Concept::Abstract& requirement) const -> Bool {
    return requirement.get_data() == get_data();
  }
};

}  // namespace Validation::ConceptTests
