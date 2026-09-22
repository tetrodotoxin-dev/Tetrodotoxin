// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/abi/core/cleanup.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"

using namespace Perimortem;

static auto get_cleanup_inventory() -> Abi::Core::Cleanup& {
  thread_local Abi::Core::Cleanup inventory;
  return inventory;
}

Abi::Core::Cleanup::~Cleanup() {
  while (latest) {
    Entry& selected = *latest;
    latest = selected.previous;
    selected.destructor();
    Perimortem::Core::Bibliotheca::remit(
        Perimortem::Core::Data::cast<U8>(&selected));
  }
}

auto Abi::Core::Cleanup::insert(Destructor destructor) -> void {
  Perimortem::Core::Bibliotheca::Allocation allocation =
      Perimortem::Core::Bibliotheca::check_out(sizeof(Entry));
  Entry* entry = Perimortem::Core::Data::cast<Entry>(allocation.ptr);
  new (entry, Perimortem::Core::Placement::Construct)
      Entry(destructor, latest);
  latest = *entry;
}

extern "C" auto perimortem_core_cleanup_register(
    Abi::Core::Cleanup::Destructor destructor) -> void {
  Core::Bibliotheca::Allocation order = Core::Bibliotheca::check_out(1);
  Core::Bibliotheca::remit(order.ptr);
  get_cleanup_inventory().insert(destructor);
}
