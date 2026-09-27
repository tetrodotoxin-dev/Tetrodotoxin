// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"
#include "tetrodotoxin/source/layouts/addressable.hpp"
#include "tetrodotoxin/source/layouts/named.hpp"
#include "tetrodotoxin/source/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::View {

// Slice borrows the available part of one requested contiguous interval.
class Slice : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "slice"_view;

  TTX_CONTRACT(Slice, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& count,
      const Language::Model::Type& result) -> Slice&;

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
  Slice(
      Tetrodotoxin::Source::Layouts::Addressable& self,
      Tetrodotoxin::Source::Layouts::Addressable& start,
      Tetrodotoxin::Source::Layouts::Addressable& count,
      const Language::Model::Type& result);

  Perimortem::Core::Static::
      Vector<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>, 3>
          parameter_entries;
  Tetrodotoxin::Source::Layouts::Named parameters;
  Tetrodotoxin::Source::Layouts::Ranged results;
  const Language::Model::Type& result_type;

  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Borrows the available part of a requested contiguous interval."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::View
