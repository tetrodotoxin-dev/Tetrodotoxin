// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"

namespace Tetrodotoxin::Package::Archive {

// Emits one canonical complete Package graph. The envelope contains only its
// coordinate, opaque Dialect members, Resources, and exact Import edges.
class Writer {
 public:
  class GraphMember {
   public:
    constexpr GraphMember(
        Perimortem::Core::View::Bytes name,
        const Tetrodotoxin::Language::Monograph& monograph)
        : name(name), monograph(monograph) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }
    constexpr auto get_monograph() const
        -> const Tetrodotoxin::Language::Monograph& {
      return monograph;
    }

   private:
    Perimortem::Core::View::Bytes name;
    const Tetrodotoxin::Language::Monograph& monograph;
  };

  Writer() = delete;

  // Writes every required section in canonical order. A body that exceeds the
  // unsigned 32 bit envelope limit logs a warning. Failure to reach the
  // measured boundary logs an error.
  static auto write(const Archive& archive)
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;

  static auto write(
      const Package::Language::Monograph& package,
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Perimortem::Core::View::Vector<GraphMember> members,
      Perimortem::Core::View::Vector<GraphImport> imports)
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;
};

}  // namespace Tetrodotoxin::Package::Archive
