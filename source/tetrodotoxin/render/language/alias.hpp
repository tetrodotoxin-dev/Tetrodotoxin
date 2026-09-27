// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/type_reference.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Render::Language {

// Render owns the authored declaration around this Type relationship. Its
// name and documentation survive independently of the reference's answer,
// so transparent Alias is composed behavior rather than a native base class.
class Alias : public Tetrodotoxin::Source::Abstract {
 public:
  TTX_CONTRACT(Alias, Tetrodotoxin::Source::Abstract);
  TTX_NAME(definition.get_name());
  TTX_DOCUMENTATION(definition.get_documentation());

  auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;
  auto get_type() const -> const Tetrodotoxin::Source::Abstract& override;
  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Tetrodotoxin::Source::Abstract& override;
  auto visit_concepts(Tetrodotoxin::Source::Abstract::Visitor visitor) const
      -> void override;
  auto bind_interface(Perimortem::System::Uuid requested) const
      -> Perimortem::Utility::Result<
          Ttx::Semantic::Negotiation::Binding,
          Ttx::Semantic::Negotiation::Binding::Failure> override;

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Language::TypeReference target) -> Alias&;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor, const Tetrodotoxin::Source::Abstract& context)
      -> Bool;

  auto link_restored(const Tetrodotoxin::Source::Abstract& context) -> Bool;

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_target_reference() const
      -> const Tetrodotoxin::Language::TypeReference& {
    return target;
  }

 private:
  Alias(
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Language::TypeReference target)
      : definition(definition), target(target) {}

  Tetrodotoxin::Language::Definition& definition;
  Tetrodotoxin::Language::TypeReference target;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>>
      selected_type;
};

}  // namespace Tetrodotoxin::Render::Language
