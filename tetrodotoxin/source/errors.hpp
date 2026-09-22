// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/source/diagnostic.hpp"

namespace Tetrodotoxin::Source {

// Errors owns reports independently of the Cursor that produced them. Each
// publication copies its message, hint and source snapshot before returning,
// allowing a parser to build messages in temporary buffers and release its
// Cursor before the host renders them. Token locators never enter this owner.
class Errors {
 public:
  Errors() : errors(arena) {}
  Errors(const Errors&) = delete;

  auto report(
      Perimortem::Core::View::Bytes source_name,
      Perimortem::Core::View::Bytes source_text,
      Perimortem::Core::Option<Anchor> anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void;
  auto get_error(Count index) const -> Perimortem::Core::Option<Diagnostic>;
  auto render_message(
      Perimortem::Memory::Allocator::Arena& arena,
      Count index,
      Perimortem::Core::View::Bytes display_source_name = {}) const
      -> Perimortem::Core::View::Bytes;
  auto get_message(Count index) const -> Perimortem::Core::View::Bytes {
    return index < errors.get_size() ? errors.at(index).diagnostic.get_message()
                                     : Perimortem::Core::View::Bytes();
  }
  auto get_source_name(Count index) const -> Perimortem::Core::View::Bytes {
    return index < errors.get_size() ? errors.at(index).name
                                     : Perimortem::Core::View::Bytes();
  }
  auto get_anchor(Count index) const -> Perimortem::Core::Option<Anchor> {
    return index < errors.get_size() ? errors.at(index).diagnostic.get_anchor()
                                     : Perimortem::Core::Option<Anchor>();
  }
  auto get_size() const -> Count { return errors.get_size(); }
  auto is_empty() const -> Bool { return get_size() == 0; }

 private:
  struct Error {
    Diagnostic diagnostic;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes text;
  };
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Managed::Vector<Error> errors;
};

}  // namespace Tetrodotoxin::Source
