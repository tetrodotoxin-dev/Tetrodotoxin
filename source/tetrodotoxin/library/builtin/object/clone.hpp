// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"
#include "tetrodotoxin/source/layouts/addressable.hpp"
#include "tetrodotoxin/source/layouts/named.hpp"
#include "tetrodotoxin/source/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::Object {

// Clone replaces one writable Object handle with an independent buffer copy.
class Clone : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "clone"_view;

  TTX_CONTRACT(Clone, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver) -> Clone&;

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
  constexpr Clone(Tetrodotoxin::Source::Layouts::Addressable& self)
      : parameters(self, 1) {}

  Tetrodotoxin::Source::Layouts::Ranged parameters;
  Tetrodotoxin::Source::Layouts::Named results;
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Replaces this Object handle with an independent copy of its buffer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Object
