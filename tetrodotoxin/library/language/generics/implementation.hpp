// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Generics {

// Implementation materializes one explicit erased value for a semantic
// requirement. The selected candidate keeps its real Object identity while its
// owner answers the higher order satisfaction query. A Terminal alone chooses
// the physical Projection carried beside that Object.
class Implementation : public Generic {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "Implementation"_view;

  Implementation(
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
      parameterization = {{Parameters::SemanticType}};
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Carries one Object whose owner satisfies an exact semantic requirement."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Generics
