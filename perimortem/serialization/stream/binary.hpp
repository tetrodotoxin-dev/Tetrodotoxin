// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/data.hpp"

namespace Perimortem::Serialization::Stream {

// Appends packed values to Dynamic::Bytes or Managed::Bytes. Existing bytes
// are never revisited. Core::Writer::Binary supports output that needs later
// patching.
template <
    Perimortem::Core::Data::ByteOrder stream_endian,
    typename storage_type>
class Binary {
 public:
  constexpr Binary(storage_type& storage) : storage(storage) {}

  auto operator<<(U8 value) -> Binary&;
  auto operator<<(U16 value) -> Binary&;
  auto operator<<(U32 value) -> Binary&;
  auto operator<<(U64 value) -> Binary&;
  auto operator<<(S8 value) -> Binary&;
  auto operator<<(S16 value) -> Binary&;
  auto operator<<(S32 value) -> Binary&;
  auto operator<<(S64 value) -> Binary&;
  auto operator<<(R32 value) -> Binary&;
  auto operator<<(R64 value) -> Binary&;
  auto operator<<(Perimortem::Core::View::Bytes blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<U8> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<U16> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<U32> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<U64> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<S8> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<S16> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<S32> blob) -> Binary&;
  auto operator<<(Perimortem::Core::View::Vector<S64> blob) -> Binary&;

 private:
  storage_type& storage;
};

}  // namespace Perimortem::Serialization::Stream
