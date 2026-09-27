// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Generics {

// Fixed accepts one element Type and one unsigned extent. The parameter domain
// expresses the nonnegative invariant directly, so materialization never
// depends on a parser specific signed conversion or a second range policy.
class Fixed : public Generic {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "Fixed"_view;

  Fixed(
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

  static constexpr Perimortem::Core::Static::Vector<Parameters, 2>
      parameterization = {{Parameters::Type, Parameters::U64}};
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Creates a fixed homogeneous range Type."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Generics
