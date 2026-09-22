// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/option.hpp"

namespace Perimortem::Abi::Core {

inline constexpr Perimortem::Core::View::Bytes cleanup_register_symbol =
    "perimortem_core_cleanup_register"_view;

// Cleanup is the ABI handler for dynamically registered destructors whose
// owned data follows Bibliotheca lifetime. Each worker owns one reverse ordered
// inventory so destruction unwinds dynamic initialization on that same worker.
//
// Registration and destruction are thread affine. A destructor must run on the
// worker that registered it, and its Object storage must never move to another
// worker. The worker's Bibliotheca must be constructed before this inventory
// and torn down only after every registered Object has been cleaned up. If the
// Librarian reclaims its slabs first, the remaining Object payloads and cleanup
// entries point into unmapped storage and teardown will segfault.
class Cleanup {
 public:
  using Destructor = void (*)();

  ~Cleanup();

  auto insert(Destructor destructor) -> void;

 private:
  class Entry {
   public:
    constexpr Entry(
        Destructor destructor,
        Perimortem::Core::Option<Entry&> previous = {})
        : destructor(destructor), previous(previous) {}

    Destructor destructor;
    Perimortem::Core::Option<Entry&> previous;
  };

  Perimortem::Core::Option<Entry&> latest;
};

}  // namespace Perimortem::Abi::Core

extern "C" auto perimortem_core_cleanup_register(
    Perimortem::Abi::Core::Cleanup::Destructor destructor) -> void;
