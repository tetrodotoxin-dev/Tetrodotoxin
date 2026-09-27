// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/attribute.hpp"
#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/shader/archive/tag.hpp"
#include "tetrodotoxin/shader/language/bridge.hpp"
#include "tetrodotoxin/shader/language/monograph.hpp"
#include "tetrodotoxin/shader/language/program.hpp"

namespace Tetrodotoxin::Shader::Archive {

// Reader reconstructs the Library child before reconnecting Shader relations.
// That order gives every Program its original concrete identity while Library
// remains the only owner that interprets its member payload.
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
  class Record {
   public:
    constexpr Record(U16 tag, Perimortem::Core::View::Bytes payload)
        : tag(tag), payload(payload) {}

    constexpr auto get_tag() const -> U16 { return tag; }
    constexpr auto get_payload() const -> Perimortem::Core::View::Bytes {
      return payload;
    }

   private:
    U16 tag;
    Perimortem::Core::View::Bytes payload;
  };

  class Definition {
   public:
    constexpr Definition(
        const Tetrodotoxin::Source::Documentation& documentation,
        Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
            attributes,
        Perimortem::Core::View::Bytes name,
        Tetrodotoxin::Language::Visibility visibility)
        : documentation(documentation),
          attributes(attributes),
          name(name),
          visibility(visibility) {}

    auto create(
        Perimortem::Memory::Allocator::Arena& arena,
        Tetrodotoxin::Source::Abstract& host) const
        -> Tetrodotoxin::Language::Definition&;

    constexpr auto get_attributes() const { return attributes; }
    constexpr auto get_visibility() const { return visibility; }

   private:
    const Tetrodotoxin::Source::Documentation& documentation;
    Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
        attributes;
    Perimortem::Core::View::Bytes name;
    Tetrodotoxin::Language::Visibility visibility;
  };

  constexpr Reader(Perimortem::Core::View::Bytes payload) : payload(payload) {}

  static auto open(Perimortem::Core::View::Bytes payload)
      -> Perimortem::Core::Option<Reader>;

  auto take(Count size)
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;
  auto read_record() -> Perimortem::Core::Option<Record>;
  auto read_u8() -> Perimortem::Core::Option<U8>;
  auto read_u32() -> Perimortem::Core::Option<U32>;
  auto read_u64() -> Perimortem::Core::Option<U64>;
  auto read_s64() -> Perimortem::Core::Option<S64>;
  auto read_r64() -> Perimortem::Core::Option<R64>;
  auto read_bytes() -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;
  auto read_documentation(Perimortem::Memory::Allocator::Arena& arena)
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Documentation&>;
  auto read_attributes(Perimortem::Memory::Allocator::Arena& arena)
      -> Perimortem::Core::Option<
          Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>>;
  auto read_definition(Perimortem::Memory::Allocator::Arena& arena)
      -> Perimortem::Core::Option<Definition>;
  auto read_program(
      Perimortem::Memory::Allocator::Arena& arena,
      Tetrodotoxin::Shader::Language::Monograph& monograph)
      -> Perimortem::Core::Option<Tetrodotoxin::Shader::Language::Program&>;
  auto read_bridge(
      Perimortem::Memory::Allocator::Arena& arena,
      Tetrodotoxin::Shader::Language::Monograph& monograph)
      -> Perimortem::Core::Option<Tetrodotoxin::Shader::Language::Bridge&>;

  constexpr auto is_complete() const -> Bool {
    return location == payload.get_size();
  }

  Perimortem::Core::View::Bytes payload;
  Count location = 0;
};

}  // namespace Tetrodotoxin::Shader::Archive
