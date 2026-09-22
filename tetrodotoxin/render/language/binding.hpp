// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/type_reference.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/addressable.hpp"

namespace Tetrodotoxin::Render::Language {

// Binding names one value required by a Render contract. It carries interface
// meaning only. Executable initialization and mutation belong to the language
// that supplies the value.
class Binding : public Tetrodotoxin::Source::Addressable {
 public:
  enum class Kind : U8 {
    Value,
    Constant,
    Push,
    Resource,
    Parameter,
  };

  enum class Access : U8 {
    None,
    Read,
    Write,
    ReadWrite,
  };

  TTX_CONTRACT(Binding, Tetrodotoxin::Source::Addressable);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Kind kind,
      Tetrodotoxin::Language::TypeReference type,
      Access access = Access::None) -> Binding&;

  static auto create_slot(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Tetrodotoxin::Source::Type& type) -> Binding&;

  static auto create_restored_slot(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Language::TypeReference type) -> Binding&;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor, const Tetrodotoxin::Source::Abstract& context)
      -> Bool;

  auto link_restored(const Tetrodotoxin::Source::Abstract& context) -> Bool;

  TTX_NAME(name);

  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override;

  auto get_type() const -> const Tetrodotoxin::Source::Abstract& override;

  constexpr auto is_linked() const -> Bool { return Bool(type); }

  auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;

  constexpr auto get_kind() const -> Kind { return kind; }

  constexpr auto get_access() const -> Access { return access; }

  constexpr auto get_type_reference() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Language::TypeReference&> {
    return type_reference.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Language::TypeReference&> { return {}; },
        [](const Tetrodotoxin::Language::TypeReference& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Language::TypeReference&> {
          return selected;
        });
  }

  constexpr auto get_definition() const
      -> Perimortem::Core::Option<const Tetrodotoxin::Language::Definition&> {
    return definition.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Language::Definition&> { return {}; },
        [](const Tetrodotoxin::Language::Definition& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Language::Definition&> {
          return selected;
        });
  }

 private:
  Binding(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::Option<Tetrodotoxin::Language::Definition&> definition,
      Kind kind,
      Access access,
      Perimortem::Core::Option<Tetrodotoxin::Language::TypeReference>
          type_reference,
      Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>>
          type)
      : name(name),
        definition(definition),
        kind(kind),
        access(access),
        type_reference(type_reference),
        type(type) {}

  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Option<Tetrodotoxin::Language::Definition&> definition;
  Kind kind;
  Access access;
  Perimortem::Core::Option<Tetrodotoxin::Language::TypeReference>
      type_reference;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>>
      type;
};

}  // namespace Tetrodotoxin::Render::Language
