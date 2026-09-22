// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Generics {

// Result is the exact value or error formula. Materialization rejects identical
// alternatives because receiving raw flow must select exactly one state.
class Result : public Generic {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "Result"_view;

  Result(
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
      parameterization = {{Parameters::Type, Parameters::Type}};
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Carries one exact value or one exact error that must be handled."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Generics
