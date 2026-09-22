// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/terminal/abi/export.hpp"
#include "tetrodotoxin/terminal/abi/projection.hpp"
#include "tetrodotoxin/terminal/abi/publication.hpp"

namespace Tetrodotoxin::Terminal::Abi {

// Products keeps the independently generated native interface surfaces and the
// exact semantic identities that use them. Instruction Terminals can consume
// the ABI agreement without becoming an owner of either header.
class Products {
 public:
  constexpr Products(
      Perimortem::Core::View::Bytes c_header,
      Perimortem::Core::View::Bytes cpp_header,
      Perimortem::Core::View::Bytes cpp_source,
      Perimortem::Core::View::Vector<Abi::Export> exports,
      Perimortem::Core::View::Vector<Abi::Publication> publications,
      Perimortem::Core::View::Vector<Abi::Projection> projections = {})
      : c_header(c_header),
        cpp_header(cpp_header),
        cpp_source(cpp_source),
        exports(exports),
        publications(publications),
        projections(projections) {}

  constexpr auto get_c_header() const -> Perimortem::Core::View::Bytes {
    return c_header;
  }

  constexpr auto get_cpp_header() const -> Perimortem::Core::View::Bytes {
    return cpp_header;
  }

  constexpr auto get_cpp_source() const -> Perimortem::Core::View::Bytes {
    return cpp_source;
  }

  constexpr auto get_exports() const
      -> Perimortem::Core::View::Vector<Abi::Export> {
    return exports;
  }

  constexpr auto get_publications() const
      -> Perimortem::Core::View::Vector<Abi::Publication> {
    return publications;
  }

  auto find_export(const Tetrodotoxin::Source::Callable& callable) const
      -> Perimortem::Core::Option<const Abi::Export&> {
    for (Count index = 0; index < exports.get_size(); index++) {
      const Abi::Export& exported = exports.get_data()[index];
      if (&exported.get_callable() == &callable) {
        return exported;
      }
    }
    return {};
  }

  auto find_publication(const Tetrodotoxin::Source::Abstract& semantic) const
      -> Perimortem::Core::Option<const Abi::Publication&> {
    for (Count index = 0; index < publications.get_size(); index++) {
      const Abi::Publication& publication = publications.get_data()[index];
      if (&publication.get_semantic() == &semantic) {
        return publication;
      }
    }
    return {};
  }

  constexpr auto get_projections() const
      -> Perimortem::Core::View::Vector<Abi::Projection> {
    return projections;
  }

 private:
  Perimortem::Core::View::Bytes c_header;
  Perimortem::Core::View::Bytes cpp_header;
  Perimortem::Core::View::Bytes cpp_source;
  Perimortem::Core::View::Vector<Abi::Export> exports;
  Perimortem::Core::View::Vector<Abi::Publication> publications;
  Perimortem::Core::View::Vector<Abi::Projection> projections;
};

}  // namespace Tetrodotoxin::Terminal::Abi
