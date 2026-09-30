// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/simulacra.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using Ttx::Semantic::Negotiation::Binding;
using namespace Tetrodotoxin::Library;

auto Simulacra::Handle::project(Abstract::Handle candidate) const
    -> Perimortem::Utility::Result<Simulacra, Failure> {
  return operations.project(source, candidate);
}

// Unknown leaves a role unsettled while allowing this projection to ask about
// other roles. An empty slot records only that no interface was acquired.
// An explicit rejection stops projection at the encountered policy.
template <typename Contract>
static auto collect(
    Abstract::Handle source,
    Option<typename Contract::Handle>& destination)
    -> Option<Binding::Failure> {
  return source.bind<Contract>().visit(
      [&](const typename Contract::Handle& selected)
          -> Option<Binding::Failure> {
        destination = selected;
        return {};
      },
      [](Binding::Failure failure) -> Option<Binding::Failure> {
        if (failure == Binding::Failure::Unknown) {
          return {};
        }
        return failure;
      });
}

auto Simulacra::project(Abstract::Handle source)
    -> Perimortem::Utility::Result<Simulacra, Failure> {
  Simulacra projection(source);

  // Ask about the boundary before asking about the imported implementation.
  // Even an acquired Import remains a dependency. Following its resolved Type
  // here would turn one edge into a copy of the other Library's declarations.
  auto failure =
      collect<Tetrodotoxin::Language::Import>(source, projection.dependency);
  if (failure) {
    return *failure;
  }
  if (projection.dependency) {
    return projection;
  }

  failure = collect<Tetrodotoxin::Language::Definition>(
      source, projection.definition);
  if (failure) {
    return *failure;
  }

  failure = collect<Tetrodotoxin::Source::Type>(source, projection.type);
  if (failure) {
    return *failure;
  }

  failure = collect<Library::Language::Initialization>(
      source, projection.initialization);
  if (failure) {
    return *failure;
  }

  failure = collect<Tetrodotoxin::Source::Callable>(source, projection.callable);
  if (failure) {
    return *failure;
  }

  failure = collect<Library::Language::Value>(source, projection.value);
  if (failure) {
    return *failure;
  }

  failure = collect<Tetrodotoxin::Source::Addressable>(source, projection.addressable);
  if (failure) {
    return *failure;
  }

  // Every Abstract already supplies navigation. A namespace can therefore
  // contribute only its source view, including an empty context, without an
  // additional role or a traversal just to decide whether it is acceptable.
  return projection;
}
