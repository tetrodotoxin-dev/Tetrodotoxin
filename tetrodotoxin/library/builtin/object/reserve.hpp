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

namespace Tetrodotoxin::Library::Builtin::Object {

class Reserve : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "reserve"_view;

  TTX_CONTRACT(Reserve, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& count,
      const Language::Model::Type& result) -> Reserve&;

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_parameters() const
      -> const Tetrodotoxin::Source::Layout& override {
    return parameters;
  }

  constexpr auto get_results() const -> const Tetrodotoxin::Source::Layout& override {
    return results;
  }

  auto accepts_receiver(
      const Tetrodotoxin::Source::Abstract& receiver,
      const Tetrodotoxin::Source::Abstract& host) const -> Bool override;

 private:
  Reserve(
      Tetrodotoxin::Source::Layouts::Addressable& self,
      Tetrodotoxin::Source::Layouts::Addressable& count,
      const Language::Model::Type& result);

  Perimortem::Core::Static::
      Vector<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>, 2>
          parameter_entries;
  Tetrodotoxin::Source::Layouts::Named parameters;
  Tetrodotoxin::Source::Layouts::Ranged results;
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Reserves at least count initialized elements and returns writable access."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Object
