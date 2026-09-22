// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/package/archive/graph_import.hpp"
#include "tetrodotoxin/package/archive/member.hpp"
#include "tetrodotoxin/package/archive/resource.hpp"

namespace Tetrodotoxin::Package::Archive {

// The source free Package terminal is a value over stable views. Archive owns
// no backing storage and applies no byte format policy. Reader retains decoded
// record ranges in its caller Arena while the input owner retains their byte
// views. Other producers keep every supplied view valid for the complete use
// of the Archive.
class Archive {
 public:
  // Counts the magic, format, reserved flags, and body size prefix.
  static constexpr Count header_size = 12;
  static constexpr U16 format = 6;

  // Creates one complete Package from views retained by its producer.
  // Construction copies only the views and preserves their supplied order.
  constexpr Archive(
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Perimortem::Core::View::Vector<Member> members,
      Perimortem::Core::View::Vector<Resource> resources = {},
      Perimortem::Core::View::Vector<GraphImport> imports = {})
      : identity(identity),
        version(version),
        members(members),
        resources(resources),
        imports(imports) {};

  constexpr auto get_identity() const -> Perimortem::Core::View::Bytes {
    return identity;
  }

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

  constexpr auto get_members() const -> Perimortem::Core::View::Vector<Member> {
    return members;
  }

  constexpr auto get_resources() const
      -> Perimortem::Core::View::Vector<Resource> {
    return resources;
  }

  constexpr auto get_imports() const
      -> Perimortem::Core::View::Vector<GraphImport> {
    return imports;
  }

 private:
  Perimortem::Core::View::Bytes identity;
  Perimortem::System::Version version;
  Perimortem::Core::View::Vector<Member> members;
  Perimortem::Core::View::Vector<Resource> resources;
  Perimortem::Core::View::Vector<GraphImport> imports;
};

}  // namespace Tetrodotoxin::Package::Archive
