// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/language/import.hpp"

namespace Tetrodotoxin::Package::Archive {

// GraphImport retains one external Type edge after terminal generation has
// resolved its locator. Source targets use another Archive member name;
// Package targets use an exact Package identity and version.
class GraphImport {
 public:
  constexpr GraphImport(
      Perimortem::Core::View::Bytes importer,
      Perimortem::Core::View::Bytes local_name,
      Tetrodotoxin::Language::Visibility visibility,
      Tetrodotoxin::Language::Import::Kind kind,
      Perimortem::Core::View::Bytes target,
      Perimortem::System::Version version = {},
      Perimortem::Core::View::Bytes route = {})
      : importer(importer),
        local_name(local_name),
        visibility(visibility),
        kind(kind),
        target(target),
        version(version),
        route(route) {}

  constexpr GraphImport(
      Perimortem::Core::View::Bytes importer,
      Perimortem::Core::View::Bytes local_name,
      Tetrodotoxin::Language::Import::Kind kind,
      Perimortem::Core::View::Bytes target,
      Perimortem::System::Version version = {})
      : GraphImport(
            importer,
            local_name,
            Tetrodotoxin::Language::Visibility::Public,
            kind,
            target,
            version) {}

  constexpr auto get_importer() const -> Perimortem::Core::View::Bytes {
    return importer;
  }
  constexpr auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }
  constexpr auto get_kind() const -> Tetrodotoxin::Language::Import::Kind {
    return kind;
  }
  constexpr auto get_visibility() const -> Tetrodotoxin::Language::Visibility {
    return visibility;
  }
  constexpr auto get_target() const -> Perimortem::Core::View::Bytes {
    return target;
  }
  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }
  constexpr auto get_route() const -> Perimortem::Core::View::Bytes {
    return route;
  }

 private:
  Perimortem::Core::View::Bytes importer;
  Perimortem::Core::View::Bytes local_name;
  Tetrodotoxin::Language::Visibility visibility;
  Tetrodotoxin::Language::Import::Kind kind;
  Perimortem::Core::View::Bytes target;
  Perimortem::System::Version version;
  Perimortem::Core::View::Bytes route;
};

}  // namespace Tetrodotoxin::Package::Archive
