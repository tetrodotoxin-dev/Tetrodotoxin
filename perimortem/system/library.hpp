// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

namespace Perimortem::System {

// Library wraps the systems native dynamic library system and manages the
// lifetime of the underlying object. Symbols can be fetched from the library
// by name but the actual symbol needs to be unsafely casted in order to call.
//
// The lifetime of any symbols are kept alive for as long as the library and
// duplicate entries get their own handle.
class Library {
 public:
  static auto open(
      Perimortem::Core::View::Bytes path,
      Perimortem::Memory::Allocator::Arena& errors)
      -> Perimortem::Utility::Result<Library, Perimortem::Core::View::Bytes>;

  Library(Library&& source);
  Library(const Library&) = delete;
  auto operator=(const Library&) -> Library& = delete;
  ~Library();

  auto symbol(
      Perimortem::Core::View::Bytes name,
      Perimortem::Memory::Allocator::Arena& errors) const
      -> Perimortem::Utility::Result<void*, Perimortem::Core::View::Bytes>;

 private:
  explicit Library(void* handle) : handle(handle) {}
  void* handle;
};

}  // namespace Perimortem::System
