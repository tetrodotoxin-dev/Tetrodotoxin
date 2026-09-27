// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/version.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/linker/fingerprint.hpp"
#include "tetrodotoxin/linker/import.hpp"

namespace Tetrodotoxin::Linker {

// Manifest is the source independent agreement beside one native artifact. It
// lets Package and later Linker consumers verify target ABI identity and retain
// the logical provider selected for every unresolved Foreign symbol.
class Manifest {
 public:
  enum class Error : U8 {
    Unknown = U8(-1),
    InvalidFormat = 0,
    UnsupportedFormat,
  };

  constexpr Manifest(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Perimortem::Core::View::Bytes artifact,
      Perimortem::Core::View::Bytes target,
      Fingerprint fingerprint,
      Perimortem::Core::View::Vector<Import> imports)
      : identity(identity),
        version(version),
        artifact(artifact),
        target(target),
        fingerprint(fingerprint),
        imports(imports) {}

  static auto write(const Manifest& manifest)
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;

  static auto read(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes bytes)
      -> Perimortem::Utility::Result<Manifest, Error>;

  constexpr auto get_identity() const -> Perimortem::Core::View::Bytes {
    return identity;
  }

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

  constexpr auto get_artifact() const -> Perimortem::Core::View::Bytes {
    return artifact;
  }

  constexpr auto get_target() const -> Perimortem::Core::View::Bytes {
    return target;
  }

  constexpr auto get_fingerprint() const -> Fingerprint { return fingerprint; }

  constexpr auto get_imports() const -> Perimortem::Core::View::Vector<Import> {
    return imports;
  }

 private:
  Perimortem::Core::View::Bytes identity;
  Perimortem::System::Version version;
  Perimortem::Core::View::Bytes artifact;
  Perimortem::Core::View::Bytes target;
  Fingerprint fingerprint;
  Perimortem::Core::View::Vector<Import> imports;
};

}  // namespace Tetrodotoxin::Linker
