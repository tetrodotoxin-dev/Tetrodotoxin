// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/import.hpp"
#include "tetrodotoxin/library/language/initialization.hpp"
#include "tetrodotoxin/library/language/value.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/callable.hpp"
#include "tetrodotoxin/source/type.hpp"
#include "tetrodotoxin/source/bound.hpp"

namespace Tetrodotoxin::Library {

// A stored Library needs to answer the questions that survive its publication
// boundary. The interpreter's classes are one way to answer them, but retaining
// their construction machinery would make every consumer rebuild the source.
// Simulacra names the equality we require between those representations.
//
// This projection is the input to packaging. It binds the available declaration
// and shape answers and leaves each referenced Abstract as an explicit edge.
// An Import ends the local projection with a dependency answer. Package can
// gather those edges across sources before assigning durable identities or
// selecting a container format. No process pointer is a serialized identity.
//
// A projection borrows one stable observation. Its providers, their code and
// every returned view must remain alive and unchanged while Package consumes
// it. Producing bytes that outlive those providers is the later serializer's
// responsibility. This object is deliberately not a restored source graph.
class Simulacra {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    0x01a08522d86b76da,
    0xb75c623dbd37ec5a,
  };

  using Failure = Ttx::Semantic::Negotiation::Binding::Failure;

  static auto project(Ttx::Concept::Abstract source)
      -> Perimortem::Utility::Result<Simulacra, Failure>;

  auto get_source() const -> Ttx::Concept::Abstract { return source; }

  auto get_import() const
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Import::Handle> {
    return dependency;
  }

  auto get_definition() const
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Definition::Handle> {
    return definition;
  }

  auto get_type() const -> Perimortem::Core::Option<Tetrodotoxin::Source::Type::Handle> {
    return type;
  }

  auto get_callable() const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Callable::Handle> {
    return callable;
  }

  auto get_value() const -> Perimortem::Core::Option<Language::Value::Handle> {
    return value;
  }

  auto get_addressable() const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Addressable::Handle> {
    return addressable;
  }

  auto get_initialization() const
      -> Perimortem::Core::Option<Language::Initialization::Handle> {
    return initialization;
  }

  // A Dialect can offer this projection as its storage policy. Package binds
  // that policy and supplies the root it is gathering, rather than knowing
  // the classes used by the Dialect's interpreter.
  struct Operations {
    auto (*project)(const void*, Ttx::Concept::Abstract)
        -> Perimortem::Utility::Result<Simulacra, Failure>;
  };

  class Handle : public Tetrodotoxin::Source::Bound<Operations> {
   public:
    using Bound::Bound;

    auto project(Ttx::Concept::Abstract candidate) const
        -> Perimortem::Utility::Result<Simulacra, Failure>;
  };

 private:
  explicit Simulacra(Ttx::Concept::Abstract source) : source(source) {}

  Ttx::Concept::Abstract source;
  Perimortem::Core::Option<Tetrodotoxin::Language::Import::Handle> dependency;
  Perimortem::Core::Option<Tetrodotoxin::Language::Definition::Handle>
      definition;
  Perimortem::Core::Option<Tetrodotoxin::Source::Type::Handle> type;
  Perimortem::Core::Option<Tetrodotoxin::Source::Callable::Handle> callable;
  Perimortem::Core::Option<Language::Value::Handle> value;
  Perimortem::Core::Option<Tetrodotoxin::Source::Addressable::Handle> addressable;
  Perimortem::Core::Option<Language::Initialization::Handle> initialization;
};

}  // namespace Tetrodotoxin::Library
