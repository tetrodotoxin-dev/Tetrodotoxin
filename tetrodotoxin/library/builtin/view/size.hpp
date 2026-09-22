// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"
#include "tetrodotoxin/source/layouts/addressable.hpp"
#include "tetrodotoxin/source/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::View {

// Size exposes the runtime element count retained by one View or Access.
class Size : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "get_size"_view;

  TTX_CONTRACT(Size, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& result) -> Size&;

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_parameters() const
      -> const Tetrodotoxin::Source::Layout& override {
    return parameters;
  }

  constexpr auto get_results() const -> const Tetrodotoxin::Source::Layout& override {
    return results;
  }

 private:
  constexpr Size(
      Tetrodotoxin::Source::Layouts::Addressable& self,
      const Language::Model::Type& result)
      : parameters(self, 1), results(result, 1) {}

  Tetrodotoxin::Source::Layouts::Ranged parameters;
  Tetrodotoxin::Source::Layouts::Ranged results;

  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Returns the number of elements available through this contiguous value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::View
