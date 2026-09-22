// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/modules/module.h"
#include "ttx/data/status.hpp"

namespace Ttx::Concept::Modules {

// Module is an acquisition policy for Concepts. Opening it supplies an owned
// Abstract directly, whether its implementation loads native code, constructs
// a managed subject or projects another system. The acquisition contract
// establishes that root's Abstract interface for immediate binding and
// navigation.
//
// The Module and each acquisition have separate lifetimes. An emitted factory
// retains the Module through its own execution and finalization, while the
// declaration acquisition can close as soon as compilation finishes. The native
// adapter uses Perimortem's worker local ownership. Other implementations
// supply their own retention policy through the C contract.
class Module {
 public:
  // Acquisition adds ownership to the existing borrowed Abstract view. Its
  // inherited operations act on the acquired root itself. The separate owner
  // pointer exists only for release and is never used to infer the root's type.
  class Acquisition : public Abstract {
   public:
    explicit Acquisition(ttx_module_acquisition value)
        : Abstract(value.root), owner(value.owner), release(value.release) {}
    Acquisition(const Acquisition&) = delete;
    auto operator=(const Acquisition&) -> Acquisition& = delete;
    Acquisition(Acquisition&& other) : Acquisition(other.take()) {}
    ~Acquisition() { close(); }

    auto take() -> ttx_module_acquisition {
      const ttx_module_acquisition result{get_abi(), owner, release};
      owner = nullptr;
      release = nullptr;
      return result;
    }

    // Clear the obligation before entering provider code so a release callback
    // can reenter close without releasing the acquisition twice. Navigation
    // and binding require the acquisition to remain open.
    auto close() -> void {
      if (release) {
        const auto previous = take();
        previous.release(previous.owner);
      }
    }

   private:
    const void* owner;
    void (*release)(const void*);
  };

  static auto load(
      Perimortem::Core::View::Bytes path,
      Perimortem::Memory::Allocator::Arena& errors)
      -> Perimortem::Utility::Result<Module, Perimortem::Core::View::Bytes>;

  explicit Module(ttx_module value) : value(value) {}
  Module(const Module& other) : value(other.value) {
    value.retain(value.source);
  }
  Module(Module&& other) : value(other.take()) {}
  auto operator=(const Module&) -> Module& = delete;
  ~Module() {
    if (value.release) {
      value.release(value.source);
    }
  }

  auto open(Semantic::Negotiation::Query host = Semantic::Negotiation::Query())
      const -> Perimortem::Utility::Result<Acquisition, Data::Status>;
  auto take() -> ttx_module {
    auto result = value;
    value = {};
    return result;
  }

 private:
  ttx_module value;
};

}  // namespace Ttx::Concept::Modules

TTX_DATA_RECORD(
    ttx_module_acquisition,
    TTX_DATA_MEMBER(ttx_module_acquisition, root),
    TTX_DATA_MEMBER(ttx_module_acquisition, owner),
    TTX_DATA_MEMBER(ttx_module_acquisition, release));

TTX_DATA_RECORD(
    ttx_module,
    TTX_DATA_MEMBER(ttx_module, source),
    TTX_DATA_MEMBER(ttx_module, retain),
    TTX_DATA_MEMBER(ttx_module, release),
    TTX_DATA_MEMBER(ttx_module, open));
