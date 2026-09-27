// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/types/contiguous.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"
#include "tetrodotoxin/source/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Fixed is one homogeneous range Type. It retains the original unsigned extent
// and element edge while Ranged exposes the repeated identity without copies.
class Fixed : public Contiguous {
 public:
  TTX_CONTRACT(Fixed, Contiguous);

  Fixed(
      Perimortem::Core::View::Bytes name,
      const Model::Type& element,
      ::U64 extent)
      : name(name),
        element(element),
        extent(extent),
        layout(element, Count(extent)) {}

  Fixed(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Model::Type& element,
      ::U64 extent,
      const Model::Type& access_type,
      const Model::Type& view_type);

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto create_fitted(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& source) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  constexpr auto get_layout() const
      -> const Tetrodotoxin::Source::Layouts::Ranged& override {
    return layout;
  }

  constexpr auto get_element_type() const -> const Model::Type& override {
    return element;
  }

  constexpr auto get_extent() const -> ::U64 { return extent; }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& element;
  ::U64 extent;
  Tetrodotoxin::Source::Layouts::Ranged layout;
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Creates a fixed homogeneous range Type."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
