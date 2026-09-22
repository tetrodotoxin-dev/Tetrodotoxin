// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/type_reference.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/layout.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/render/language/monograph.hpp"
#include "tetrodotoxin/render/language/stage.hpp"
#include "tetrodotoxin/render/language/structure.hpp"
#include "tetrodotoxin/shader/language/binding.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Shader::Language {

// Program is the Library Composite authored inside one Shader definition. It
// owns executable Stage Functions and the generated Library surface inherited
// from one exact Render contract. Uniform Fields remain authored identities in
// Parameters while Instance gives CPU code one concrete mutable Object.
class Program : public Tetrodotoxin::Library::Language::Types::Structure {
 public:
  TTX_CONTRACT(Program, Tetrodotoxin::Library::Language::Types::Structure);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Language::TypeReference contract,
      Tetrodotoxin::Source::Abstract& context) -> Program&;

  static auto create_restored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Language::TypeReference contract,
      Tetrodotoxin::Source::Abstract& context) -> Program&;

  auto initialize_runtime_surface() -> Bool;
  auto restore_runtime_surface() -> Bool;
  auto complete_authored_body() -> void;

  auto retain_shader_binding(
      Tetrodotoxin::Library::Language::Field& field,
      Tetrodotoxin::Render::Language::Binding::Kind kind,
      Tetrodotoxin::Render::Language::Binding::Access access =
          Tetrodotoxin::Render::Language::Binding::Access::None,
      Perimortem::Core::Option<Tetrodotoxin::Library::Language::Field&>
          instance_field = {}) -> void;

  auto retain_instance_resource(
      Tetrodotoxin::Library::Language::Field& field,
      Tetrodotoxin::Library::Language::TypeReference runtime_type)
      -> Perimortem::Core::Option<Tetrodotoxin::Library::Language::Field&>;

  auto retain_uniform(Tetrodotoxin::Library::Language::Field& field) -> void;

  auto retain_stage(Tetrodotoxin::Library::Language::Function& function)
      -> void;

  auto compose_contract(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;
  auto compose_contract_restored() -> Bool;

  auto validate_contract(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;
  auto validate_contract_restored() -> Bool;

  auto satisfies(const Tetrodotoxin::Source::Abstract& requirement) const
      -> Bool override;

  constexpr auto get_contract_reference() const
      -> const Tetrodotoxin::Language::TypeReference& {
    return contract;
  }

  constexpr auto get_contract() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Render::Language::Monograph&> {
    return contract_type.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Render::Language::Monograph&> {
          return {};
        },
        [](const Tetrodotoxin::Source::Reference<
            const Tetrodotoxin::Render::Language::Monograph>& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Render::Language::Monograph&> {
          return selected.get();
        });
  }

  constexpr auto get_bindings() const
      -> Perimortem::Core::View::Vector<Binding> {
    return bindings;
  }

  constexpr auto get_uniforms() const -> Perimortem::Core::View::Vector<
      Tetrodotoxin::Source::Reference<Tetrodotoxin::Library::Language::Field>> {
    return uniforms;
  }

  constexpr auto edit_parameters()
      -> Tetrodotoxin::Library::Language::Types::Structure& {
    return parameters->get();
  }

  constexpr auto get_parameters() const
      -> const Tetrodotoxin::Library::Language::Types::Structure& {
    return parameters->get();
  }

  constexpr auto get_instance() const
      -> const Tetrodotoxin::Library::Language::Types::Object& {
    return instance->get();
  }

  constexpr auto get_parameters_field() const
      -> const Tetrodotoxin::Library::Language::Field& {
    return parameters_field->get();
  }

  constexpr auto get_instance_parameters_field() const
      -> const Tetrodotoxin::Library::Language::Field& {
    return instance_parameters_field->get();
  }

 private:
  class StageBody {
   public:
    constexpr StageBody(Tetrodotoxin::Library::Language::Function& function)
        : function(function) {}

    Tetrodotoxin::Source::Reference<Tetrodotoxin::Library::Language::Function> function;
  };

  class InheritedType {
   public:
    constexpr InheritedType(
        const Tetrodotoxin::Render::Language::Structure& requirement,
        Tetrodotoxin::Library::Language::Types::Structure& implementation)
        : requirement(requirement), implementation(implementation) {}

    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Render::Language::Structure>
        requirement;
    Tetrodotoxin::Source::Reference<Tetrodotoxin::Library::Language::Types::Structure>
        implementation;
  };

  Program(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Language::TypeReference contract,
      Tetrodotoxin::Source::Abstract& context)
      : Tetrodotoxin::Library::Language::Types::Structure(
            domain,
            definition,
            False),
        domain(domain),
        contract(contract),
        context(context),
        bindings(domain),
        uniforms(domain),
        stages(domain),
        inherited_types(domain) {}

  auto project_contract(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;
  auto project_structure(
      const Tetrodotoxin::Render::Language::Structure& requirement,
      Tetrodotoxin::Library::Language::Types::Composite& host) -> Bool;
  auto project_binding(
      const Tetrodotoxin::Render::Language::Binding& requirement,
      Tetrodotoxin::Library::Language::Types::Composite& host,
      Tetrodotoxin::Library::Language::Writability writability,
      Bool retain_role) -> Bool;
  auto project_type(const Tetrodotoxin::Source::Abstract& requirement) const
      -> Perimortem::Core::Option<
          const Tetrodotoxin::Library::Language::Model::Type&>;
  auto restore_projected_structure(
      const Tetrodotoxin::Render::Language::Structure& requirement,
      Tetrodotoxin::Library::Language::Types::Composite& host) -> Bool;
  auto restore_stage(
      Tetrodotoxin::Library::Language::Function& function,
      const Tetrodotoxin::Render::Language::Stage& stage) -> Bool;

  Perimortem::Memory::Allocator::Arena& domain;
  Tetrodotoxin::Language::TypeReference contract;
  Tetrodotoxin::Source::Abstract& context;
  Perimortem::Memory::Managed::Vector<Binding> bindings;
  Perimortem::Memory::Managed::Vector<
      Tetrodotoxin::Source::Reference<Tetrodotoxin::Library::Language::Field>>
      uniforms;
  Perimortem::Memory::Managed::Vector<StageBody> stages;
  Perimortem::Memory::Managed::Vector<InheritedType> inherited_types;
  Perimortem::Core::Option<Tetrodotoxin::Source::Reference<
      Tetrodotoxin::Library::Language::Types::Structure>>
      parameters;
  Perimortem::Core::Option<
      Tetrodotoxin::Source::Reference<Tetrodotoxin::Library::Language::Types::Object>>
      instance;
  Perimortem::Core::Option<
      Tetrodotoxin::Source::Reference<Tetrodotoxin::Library::Language::Field>>
      parameters_field;
  Perimortem::Core::Option<
      Tetrodotoxin::Source::Reference<Tetrodotoxin::Library::Language::Field>>
      instance_parameters_field;
  Perimortem::Core::Option<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Render::Language::Monograph>>
      contract_type;
  Bool composed = False;
};

}  // namespace Tetrodotoxin::Shader::Language
