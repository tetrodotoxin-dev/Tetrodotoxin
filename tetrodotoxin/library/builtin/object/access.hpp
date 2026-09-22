// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"
#include "tetrodotoxin/source/layouts/addressable.hpp"
#include "tetrodotoxin/source/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::Object {

class Access : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "get_access"_view;

  TTX_CONTRACT(Access, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& result) -> Access&;

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
  constexpr Access(
      Tetrodotoxin::Source::Layouts::Addressable& self,
      const Language::Model::Type& result)
      : parameters(self, 1), results(result, 1) {}

  Tetrodotoxin::Source::Layouts::Ranged parameters;
  Tetrodotoxin::Source::Layouts::Ranged results;
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Borrows writable access to this Object buffer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Object
