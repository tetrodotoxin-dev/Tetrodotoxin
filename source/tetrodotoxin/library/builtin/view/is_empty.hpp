// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"
#include "tetrodotoxin/source/layouts/addressable.hpp"
#include "tetrodotoxin/source/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::View {

// IsEmpty reports whether one View or Access contains no elements.
class IsEmpty : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "is_empty"_view;

  TTX_CONTRACT(IsEmpty, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& result) -> IsEmpty&;

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_parameters() const
      -> const Tetrodotoxin::Source::Layout& override {
    return parameters;
  }

  constexpr auto get_results() const -> const Tetrodotoxin::Source::Layout& override {
    return results;
  }

  auto fold_call(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::Option<const Language::Model::Pack&> receiver,
      const Language::Model::Pack& arguments) const
      -> Perimortem::Core::Option<Language::Model::Pack&> override;

 private:
  constexpr IsEmpty(
      Tetrodotoxin::Source::Layouts::Addressable& self,
      const Language::Model::Type& result)
      : parameters(self, 1), results(result, 1), result_type(result) {}

  Tetrodotoxin::Source::Layouts::Ranged parameters;
  Tetrodotoxin::Source::Layouts::Ranged results;
  const Language::Model::Type& result_type;
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Returns whether this contiguous value contains no elements."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::View
