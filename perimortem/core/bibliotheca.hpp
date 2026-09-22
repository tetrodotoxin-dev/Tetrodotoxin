// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Core {

// The primary memory manager used by all systems in the Perimortem engine.
//
// Provides Thread local allocator for isolating and caching thread allocations.
// Can be mixed with the standard library but using STL objects with the
// Bibliotheca is not recommended.
//
// Any memory fetched from the Bibliotheca is guaranteed to be cleaned up on
// thread exit. Until thread exit memory is perserved and is allocated into
// power of 2 chunks.
//
// For ideal performance threads should have stable allocation and deallocation
// patterns but this isn't a hard requirement.
class Bibliotheca {
 public:
  // The legal amount that algorithms are able to underwrite the allocated
  // buffer.
  //
  // For x86_64 the underwrite buffer is guaranteed to be on a separate cache
  // line from the data pointer address making it can be useful for storing
  // infrequently used data related to the allocation or for algorithms that
  // avoid a copy by using a bit of the underwrite buffer.
  //
  // Algorithms and types that use the underwrite buffer are incompatable with
  // the C++ standard library so it should be used sparingly.
  static constexpr auto legal_underwrite_size = 16;

  // Every corpus begins after one cache line sized Preface. Slab pages preserve
  // that boundary, so consumers may construct values with alignment no greater
  // than this contract.
  static constexpr auto allocation_alignment = 64;

  struct Allocation {
    U8* ptr;
    Count capacity;
  };

  // Creates a free entry which can be used.
  static auto check_out(Count requested_bytes) -> Allocation;

  // Adds a reservation to the block.
  static auto reserve(U8* entry) -> Count;

  // Returns the number of active reservations on the block.
  static auto reservation_count(U8* entry) -> Count;

  // Returns the usable byte capacity selected when this block was checked out.
  // An empty entry has zero capacity.
  static auto capacity(U8* entry) -> Count;

  // Associates one immutable runtime descriptor with a checked out Object
  // block without consuming the algorithm underwrite region.
  static auto bind_object(U8* entry, const void* descriptor) -> void;

  // Returns the descriptor associated with an Object block or no descriptor
  // for an ordinary allocation.
  static auto get_object(U8* entry) -> const void*;

  // Removes a reservation from the block.
  // If the number of reservations is zero then the block is checked in to the
  // Bibliotheca for future use.
  static auto remit(U8* entry) -> Count;

  // Methods for analyzing the state of the Bibliotheca.
  static auto reserved_memory() -> Count;
  static auto free_memory() -> Count;
  static auto allocated_memory() -> Count;
  static auto check_out_requests() -> Count;
  static auto allocation_requests() -> Count;
  static auto slab_requests() -> Count;
};

}  // namespace Perimortem::Core
