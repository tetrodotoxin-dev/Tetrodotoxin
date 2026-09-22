// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::System {

// Supplies fast thread local pseudorandom values and direct platform seed
// entropy through separate calls. Normal runtime work should use generate(),
// which advances a Philox counter without sharing mutable state between
// threads. read_entropy() crosses into the platform entropy source and is
// reserved for seeding or values whose unpredictability matters more than
// throughput.
class Random {
 public:
  static auto generate() -> U64;

  static auto read_entropy() -> U64;
};

}  // namespace Perimortem::System
