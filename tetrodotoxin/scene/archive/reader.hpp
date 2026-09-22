// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"

namespace Tetrodotoxin::Scene::Archive {

// Reader reconstructs the Library child and Scene instance before reconnecting
// Signals and lifecycle roles. Every relationship therefore selects a fresh
// identity created by its real owner in the same restoration Arena.
class Reader {
 public:
  static auto restore(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes payload,
      const Tetrodotoxin::Source::Abstract& language,
      const Tetrodotoxin::Library::Dialect& library,
      Tetrodotoxin::Source::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Abstract&>;

 private:
  constexpr Reader(Perimortem::Core::View::Bytes payload) : payload(payload) {}

  static auto open(Perimortem::Core::View::Bytes payload)
      -> Perimortem::Core::Option<Reader>;

  auto take(Count size)
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;
  auto read_u8() -> Perimortem::Core::Option<U8>;
  auto read_u32() -> Perimortem::Core::Option<U32>;
  auto read_bytes() -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;
  auto read_documentation(Perimortem::Memory::Allocator::Arena& arena)
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Documentation&>;

  constexpr auto is_complete() const -> Bool {
    return location == payload.get_size();
  }

  Perimortem::Core::View::Bytes payload;
  Count location = 0;
};

}  // namespace Tetrodotoxin::Scene::Archive
