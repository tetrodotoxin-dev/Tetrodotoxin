// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Generics {

// Access is the writable contiguous storage formula. Its materialized Types
// retain the element identity while Access keeps only the immutable rule that
// constructs them. Access establishes write capability but does not claim
// noalias.
class Access : public Generic {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "Access"_view;

  Access(
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
    "Provides writable access to contiguous values."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Generics
