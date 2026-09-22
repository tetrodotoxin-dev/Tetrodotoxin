// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Generics {

// Object is the managed buffer formula. Each materialization retains one exact
// element Type and publishes its generated capacity and borrow operations.
class Object : public Generic {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "Object"_view;

  Object(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Abstract& context)
      : Generic(domain, context) {}

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_parameterization() const
      -> Perimortem::Core::View::Vector<Parameters> override {
    return parameterization;
  }

 private:
  auto create(Perimortem::Core::View::Vector<Argument> arguments) const
      -> Perimortem::Core::Option<const Model::Type&> override;

  static constexpr Perimortem::Core::Static::Vector<Parameters, 1>
      parameterization = {{Parameters::Type}};
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Creates an empty-capable managed buffer Type."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Generics
