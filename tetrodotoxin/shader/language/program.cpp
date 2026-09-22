// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/language/program.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/shader/language/contract.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

static auto make_reference(Allocator::Arena& domain, View::Bytes route)
    -> Library::Language::TypeReference {
  return Library::Language::TypeReference::create(
      domain.proxy(route), Anchor::create(Span()), Token());
}

static auto find_structure(
    Library::Language::Types::Composite& host,
    View::Bytes name) -> Option<Library::Language::Types::Structure&> {
  Option<Library::Language::Types::Structure&> selected;
  for (const Reference<Abstract>& declaration : host.get_types()) {
    auto candidate =
        declaration.get().select<Library::Language::Types::Structure>();
    if (candidate && !candidate->is<Library::Language::Types::Object>() &&
        candidate->get_name() == name) {
      BAIL_IF(selected);
      selected = *candidate;
    }
  }
  return selected;
}

static auto find_field(
    Library::Language::Types::Composite& host,
    View::Bytes name) -> Option<Library::Language::Field&> {
  Option<Library::Language::Field&> selected;
  for (const Reference<Abstract>& declaration : host.get_addressables()) {
    auto candidate = declaration.get().select<Library::Language::Field>();
    if (candidate && candidate->get_name() == name) {
      BAIL_IF(selected);
      selected = *candidate;
    }
  }
  return selected;
}

static auto find_function(
    Library::Language::Types::Composite& host,
    View::Bytes name) -> Option<Library::Language::Function&> {
  Option<Library::Language::Function&> selected;
  for (const Reference<Abstract>& declaration : host.get_callables()) {
    auto candidate = declaration.get().select<Library::Language::Function>();
    if (candidate && candidate->get_name() == name) {
      BAIL_IF(selected);
      selected = *candidate;
    }
  }
  return selected;
}

auto Shader::Language::Program::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Tetrodotoxin::Language::TypeReference contract,
    Abstract& context) -> Program& {
  return domain.construct_from<Program>(
      [&]() { return Program(domain, definition, contract, context); });
}

auto Shader::Language::Program::create_restored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Tetrodotoxin::Language::TypeReference contract,
    Abstract& context) -> Program& {
  return domain.construct_from<Program>(
      [&]() { return Program(domain, definition, contract, context); });
}

auto Shader::Language::Program::initialize_runtime_surface() -> Bool {
  BAIL_IF(
      parameters || instance || parameters_field || instance_parameters_field);

  auto& parameters_definition =
      Tetrodotoxin::Language::Definition::create_synthetic(
          domain, Tetrodotoxin::Source::Documentation::get_empty(), *this, "Parameters"_view,
          Tetrodotoxin::Language::Visibility::Public,
          get_definition().get_authored().get_anchor());
  auto& parameters_type = Library::Language::Types::Structure::create_authored(
      domain, parameters_definition);
  BAIL_IF(!retain_definition(parameters_type, Category::Type, True));
  parameters = Reference<Library::Language::Types::Structure>(parameters_type);

  auto& instance_definition =
      Tetrodotoxin::Language::Definition::create_synthetic(
          domain, Tetrodotoxin::Source::Documentation::get_empty(), *this, "Material"_view,
          Tetrodotoxin::Language::Visibility::Public,
          get_definition().get_authored().get_anchor());
  auto& instance_type = Library::Language::Types::Object::create_synthetic(
      domain, instance_definition);
  BAIL_IF(!retain_definition(instance_type, Category::Type, True));
  instance = Reference<Library::Language::Types::Object>(instance_type);

  auto& program_field_definition =
      Tetrodotoxin::Language::Definition::create_synthetic(
          domain, Tetrodotoxin::Source::Documentation::get_empty(), *this, "parameters"_view,
          Tetrodotoxin::Language::Visibility::Public, Anchor::create(Span()));
  auto& program_field = Library::Language::Field::create(
      domain, program_field_definition, Library::Language::Writability::Full,
      make_reference(domain, "Parameters"_view), {});
  BAIL_IF(!retain_definition(program_field, Category::Addressable, True));
  parameters_field = Reference<Library::Language::Field>(program_field);

  auto& instance_field_definition =
      Tetrodotoxin::Language::Definition::create_synthetic(
          domain, Tetrodotoxin::Source::Documentation::get_empty(), instance_type, "parameters"_view,
          Tetrodotoxin::Language::Visibility::Public, Anchor::create(Span()));
  auto& instance_field = Library::Language::Field::create(
      domain, instance_field_definition,
      Library::Language::Writability::Internal,
      make_reference(domain, "Parameters"_view), {});
  BAIL_IF(!instance_type.retain_definition(
      instance_field, Category::Addressable, True));
  instance_parameters_field =
      Reference<Library::Language::Field>(instance_field);
  return True;
}

auto Shader::Language::Program::restore_runtime_surface() -> Bool {
  BAIL_IF(
      parameters || instance || parameters_field || instance_parameters_field);
  for (const Reference<Abstract>& declaration : get_declarations()) {
    if (declaration.get().get_name() == "Parameters"_view) {
      auto selected =
          declaration.get().select<Library::Language::Types::Structure>();
      BAIL_IF(!selected || selected->is<Library::Language::Types::Object>());
      parameters = Reference<Library::Language::Types::Structure>(*selected);
    } else if (declaration.get().get_name() == "Material"_view) {
      auto selected =
          declaration.get().select<Library::Language::Types::Object>();
      BAIL_IF(!selected);
      instance = Reference<Library::Language::Types::Object>(*selected);
    } else if (declaration.get().get_name() == "parameters"_view) {
      auto selected = declaration.get().select<Library::Language::Field>();
      BAIL_IF(!selected);
      parameters_field = Reference<Library::Language::Field>(*selected);
    }
  }
  BAIL_IF(!parameters || !instance || !parameters_field);

  Count instance_parameters = 0;
  for (const Reference<Abstract>& declaration :
       instance->get().get_declarations()) {
    auto field = declaration.get().select<Library::Language::Field>();
    if (field && field->get_name() == "parameters"_view) {
      instance_parameters++;
      instance_parameters_field = Reference<Library::Language::Field>(*field);
    }
  }
  return instance_parameters == 1 && instance_parameters_field;
}

auto Shader::Language::Program::complete_authored_body() -> void {
  parameters->get().complete_body();
  instance->get().complete_body();
  complete_body();
}

auto Shader::Language::Program::retain_shader_binding(
    Library::Language::Field& field,
    Render::Language::Binding::Kind kind,
    Render::Language::Binding::Access access,
    Option<Library::Language::Field&> instance_field) -> void {
  if (!instance_field && kind == Render::Language::Binding::Kind::Resource) {
    instance_field = find_field(instance->get(), field.get_name());
  }
  bindings.insert(Binding(field, kind, access, instance_field));
}

auto Shader::Language::Program::retain_instance_resource(
    Library::Language::Field& field,
    Library::Language::TypeReference runtime_type)
    -> Option<Library::Language::Field&> {
  const Tetrodotoxin::Language::Definition& authored = field.get_definition();
  auto& definition = Tetrodotoxin::Language::Definition::create_synthetic(
      domain, authored.get_documentation(), instance->get(),
      domain.proxy(field.get_name()), authored.get_visibility(),
      Anchor::create(Span()), authored.get_attributes());
  auto& runtime_field = Library::Language::Field::create(
      domain, definition, Library::Language::Writability::Internal,
      runtime_type, {});
  BAIL_IF(!instance->get().retain_definition(
      runtime_field, Library::Language::Types::Composite::Category::Addressable,
      definition.is_published()));
  return runtime_field;
}

auto Shader::Language::Program::retain_uniform(Library::Language::Field& field)
    -> void {
  uniforms.insert(field);
}

auto Shader::Language::Program::retain_stage(
    Library::Language::Function& function) -> void {
  stages.insert(StageBody(function));
}

auto Shader::Language::Program::project_binding(
    const Render::Language::Binding& requirement,
    Library::Language::Types::Composite& host,
    Library::Language::Writability writability,
    Bool retain_role) -> Bool {
  auto definition = requirement.get_definition();
  BAIL_IF(!requirement.is_linked());
  auto type = project_type(requirement.get_type());
  BAIL_IF(!definition || !type);

  auto& projected_definition =
      Tetrodotoxin::Language::Definition::create_synthetic(
          domain, definition->get_documentation(), host,
          domain.proxy(requirement.get_name()), definition->get_visibility(),
          Anchor::create(Span()), definition->get_attributes());
  auto& field = Library::Language::Field::create_generated(
      domain, projected_definition, writability, *type);
  BAIL_IF(!host.retain_definition(
      field, Library::Language::Types::Composite::Category::Addressable,
      projected_definition.is_published()));
  if (retain_role) {
    retain_shader_binding(
        field, requirement.get_kind(), requirement.get_access());
  }
  return True;
}

auto Shader::Language::Program::project_type(const Abstract& requirement) const
    -> Option<const Library::Language::Model::Type&> {
  auto exact = requirement.select<Tetrodotoxin::Source::Type>();
  BAIL_IF(!exact);
  auto direct = exact->select<Library::Language::Model::Type>();
  if (direct) {
    return *direct;
  }
  auto render = exact->select<Render::Language::Structure>();
  BAIL_IF(!render);
  for (const InheritedType& inherited : inherited_types.get_view()) {
    if (&inherited.requirement.get() == &*render) {
      return inherited.implementation.get();
    }
  }
  return {};
}

auto Shader::Language::Program::project_structure(
    const Render::Language::Structure& requirement,
    Library::Language::Types::Composite& host) -> Bool {
  auto& definition = Tetrodotoxin::Language::Definition::create_synthetic(
      domain, requirement.get_documentation(), host,
      domain.proxy(requirement.get_name()),
      requirement.get_definition().get_visibility(), Anchor::create(Span()),
      requirement.get_definition().get_attributes());
  auto& projected =
      Library::Language::Types::Structure::create_authored(domain, definition);
  BAIL_IF(!host.retain_definition(
      projected, Library::Language::Types::Composite::Category::Type,
      definition.is_published()));
  inherited_types.insert(InheritedType(requirement, projected));

  for (const Reference<Abstract>& declaration : requirement.get_types()) {
    auto nested = declaration.get().select<Render::Language::Structure>();
    BAIL_IF(!nested || !project_structure(*nested, projected));
  }
  for (const Reference<Abstract>& declaration :
       requirement.get_addressables()) {
    auto binding = declaration.get().select<Render::Language::Binding>();
    BAIL_IF(
        !binding ||
        binding->get_kind() != Render::Language::Binding::Kind::Value ||
        !project_binding(
            *binding, projected, Library::Language::Writability::Internal,
            False));
  }
  projected.complete_body();
  return True;
}

auto Shader::Language::Program::project_contract(Cursor& cursor) -> Bool {
  auto selected = contract.resolve(cursor, context);
  auto render_contract = selected
                             ? selected->select<Render::Language::Monograph>()
                             : Option<const Render::Language::Monograph&>();
  if (!render_contract) {
    cursor.create_expression_error(
        contract.get_anchor(),
        "Shader relationship must select one Pipeline source."_view);
    return False;
  }
  contract_type =
      Reference<const Render::Language::Monograph>(*render_contract);

  for (const Reference<Abstract>& declaration : render_contract->get_types()) {
    auto structure = declaration.get().select<Render::Language::Structure>();
    BAIL_IF(!structure || !project_structure(*structure, *this));
  }
  for (const Reference<Abstract>& declaration :
       render_contract->get_addressables()) {
    auto binding = declaration.get().select<Render::Language::Binding>();
    BAIL_IF(
        !binding ||
        !project_binding(
            *binding, *this, Library::Language::Writability::Full, True));
  }
  retain_shader_binding(
      parameters_field->get(), Render::Language::Binding::Kind::Push);

  for (Count stage_index = 0; stage_index < stages.get_size(); stage_index++) {
    StageBody& body = stages[stage_index];
    Option<const Render::Language::Stage&> required;
    for (const Reference<Abstract>& declaration :
         render_contract->get_callables()) {
      auto stage = declaration.get().select<Render::Language::Stage>();
      if (stage && stage->get_name() == body.function.get().get_name()) {
        BAIL_IF(required);
        required = *stage;
      }
    }
    if (!required) {
      cursor.create_expression_error(
          body.function.get().get_anchor(),
          "Shader Stage body has no matching Pipeline Stage requirement."_view,
          "Name one executable body for each Stage selected by the contract."_view);
      return False;
    }
  }

  complete_body();
  composed = True;
  return True;
}

auto Shader::Language::Program::compose_contract(Cursor& cursor) -> Bool {
  return composed || project_contract(cursor);
}

auto Shader::Language::Program::restore_projected_structure(
    const Render::Language::Structure& requirement,
    Library::Language::Types::Composite& host) -> Bool {
  auto projected = find_structure(host, requirement.get_name());
  BAIL_IF(!projected);
  inherited_types.insert(InheritedType(requirement, *projected));

  for (const Reference<Abstract>& declaration : requirement.get_types()) {
    auto nested = declaration.get().select<Render::Language::Structure>();
    BAIL_IF(!nested || !restore_projected_structure(*nested, *projected));
  }
  for (const Reference<Abstract>& declaration :
       requirement.get_addressables()) {
    auto binding = declaration.get().select<Render::Language::Binding>();
    auto field = binding ? find_field(*projected, binding->get_name())
                         : Option<Library::Language::Field&>();
    auto type = binding ? project_type(binding->get_type())
                        : Option<const Library::Language::Model::Type&>();
    BAIL_IF(
        !binding ||
        binding->get_kind() != Render::Language::Binding::Kind::Value ||
        !field || !type || !field->retain_generated_type(*type));
  }
  return True;
}

auto Shader::Language::Program::restore_stage(
    Library::Language::Function& function,
    const Render::Language::Stage& stage) -> Bool {
  auto restore = [&](Library::Language::Model::Layout& target,
                     const Render::Language::Layout& source) -> Bool {
    auto slots = source.get_slots();
    BAIL_IF(target.get_size() != slots.get_size());
    for (Count index = 0; index < slots.get_size(); index++) {
      const Render::Language::Layout::Slot& slot = slots.get_data()[index];
      BAIL_IF(target.get_declared_name(index) != slot.get_name());
    }
    return True;
  };

  return restore(
             function.edit_signature().edit_parameters(),
             stage.get_parameter_layout()) &&
         restore(
             function.edit_signature().edit_results(),
             stage.get_result_layout());
}

auto Shader::Language::Program::compose_contract_restored() -> Bool {
  if (composed) {
    return True;
  }
  auto selected = contract.resolve_restored(context);
  auto render_contract = selected
                             ? selected->select<Render::Language::Monograph>()
                             : Option<const Render::Language::Monograph&>();
  BAIL_IF(!render_contract);
  if (!parameters || !instance || !parameters_field) {
    BAIL_IF(!restore_runtime_surface());
  }
  contract_type =
      Reference<const Render::Language::Monograph>(*render_contract);

  for (const Reference<Abstract>& declaration : render_contract->get_types()) {
    auto structure = declaration.get().select<Render::Language::Structure>();
    BAIL_IF(!structure || !restore_projected_structure(*structure, *this));
  }
  for (const Reference<Abstract>& declaration :
       render_contract->get_addressables()) {
    auto requirement = declaration.get().select<Render::Language::Binding>();
    Option<Library::Language::Field&> field;
    if (requirement) {
      for (Count index = 0; index < bindings.get_size(); index++) {
        Binding& binding = bindings[index];
        if (binding.get_kind() == requirement->get_kind() &&
            binding.get_access() == requirement->get_access() &&
            binding.get_field().get_name() == requirement->get_name()) {
          BAIL_IF(field);
          field = binding.edit_field();
        }
      }
    }
    auto type = requirement ? project_type(requirement->get_type())
                            : Option<const Library::Language::Model::Type&>();
    BAIL_IF(
        !requirement || !field || !type ||
        !field->retain_generated_type(*type));
  }
  for (const Reference<Abstract>& declaration :
       render_contract->get_callables()) {
    auto stage = declaration.get().select<Render::Language::Stage>();
    auto function = stage ? find_function(*this, stage->get_name())
                          : Option<Library::Language::Function&>();
    BAIL_IF(!stage || !function || !restore_stage(*function, *stage));
  }
  composed = True;
  return True;
}

auto Shader::Language::Program::validate_contract(Cursor& cursor) -> Bool {
  BAIL_IF(!contract_type);
  Contract negotiator;
  return negotiator.validate(cursor, contract_type->get(), *this);
}

auto Shader::Language::Program::validate_contract_restored() -> Bool {
  BAIL_IF(!contract_type);
  Contract negotiator;
  return negotiator.validate_restored(contract_type->get(), *this);
}

auto Shader::Language::Program::satisfies(const Abstract& requirement) const
    -> Bool {
  return contract_type &&
         &contract_type->get().resolve() == &requirement.resolve();
}
