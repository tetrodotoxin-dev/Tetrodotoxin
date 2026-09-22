// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/language/contract.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/render/language/attributes.hpp"
#include "tetrodotoxin/render/language/binding.hpp"
#include "tetrodotoxin/render/language/stage.hpp"
#include "tetrodotoxin/source/interfaces/callable.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

static auto find_function(
    const Shader::Language::Program& program,
    View::Bytes name) -> Option<const Library::Language::Function&> {
  for (const Reference<Abstract>& candidate : program.get_callables()) {
    auto function = candidate.get().select<Library::Language::Function>();
    if (function && function->get_name() == name) {
      return *function;
    }
  }
  return {};
}

static auto find_binding(
    const Shader::Language::Program& program,
    View::Bytes name) -> Option<const Shader::Language::Binding&> {
  auto bindings = program.get_bindings();
  for (Count index = 0; index < bindings.get_size(); index++) {
    const Shader::Language::Binding& binding = bindings.get_data()[index];
    if (binding.get_field().get_name() == name) {
      return binding;
    }
  }
  return {};
}

static auto slot_failure(
    const Library::Language::Model::Layout& supplied,
    const Render::Language::Layout& required) -> View::Bytes {
  for (const Render::Language::Layout::Slot& requirement :
       required.get_slots()) {
    Option<Count> selected;
    for (Count index = 0; index < supplied.get_size(); index++) {
      auto name = supplied.get_name(index);
      if (name && *name == requirement.get_name()) {
        selected = index;
        break;
      }
    }
    if (!selected) {
      return "The restored Stage Layout is missing one named Pipeline slot."_view;
    }
  }
  return {};
}

class Evaluation {
 public:
  constexpr Evaluation(
      Tetrodotoxin::Source::Interface::Relation relation,
      View::Bytes failure = {})
      : relation(relation), failure(failure) {}

  Tetrodotoxin::Source::Interface::Relation relation;
  View::Bytes failure;

  static auto compatible_binding_type(
      const Abstract& requirement,
      const Abstract& candidate) -> Bool {
    auto requirement_type = requirement.select<Tetrodotoxin::Source::Type>();
    auto candidate_type = candidate.select<Tetrodotoxin::Source::Type>();
    BAIL_IF(!requirement_type || !candidate_type);
    const Abstract& required = requirement_type->resolve();
    const Abstract& supplied = candidate_type->resolve();
    if (&required == &supplied) {
      return True;
    }

    auto render = required.select<Render::Language::Structure>();
    auto library = supplied.select<Library::Language::Types::Structure>();
    return render && library &&
           render->get_layout().fits(library->get_layout()) &&
           library->get_layout().fits(render->get_layout());
  }
};

static auto evaluate(const Abstract& requirement, const Abstract& candidate)
    -> Evaluation {
  auto render = requirement.resolve().select<Render::Language::Monograph>();
  auto shader = candidate.resolve().select<Shader::Language::Program>();
  if (!render || !shader) {
    return Evaluation(
        Tetrodotoxin::Source::Interface::Relation::Rejected,
        "The restored relationship no longer selects Pipeline and Shader owners."_view);
  }

  // Callable negotiation establishes shared value flow first. Render then adds
  // Stage and slot policy that Layout fitting deliberately omits.
  Tetrodotoxin::Source::Interfaces::Callable callable_interface;
  for (const Reference<Abstract>& entry : render->get_callables()) {
    auto required = entry.get().select<Render::Language::Stage>();
    if (!required) {
      return Evaluation(
          Tetrodotoxin::Source::Interface::Relation::Rejected,
          "The restored Pipeline callable is not one Stage."_view);
    }
    auto supplied = find_function(*shader, required->get_name());
    if (!supplied) {
      return Evaluation(
          Tetrodotoxin::Source::Interface::Relation::Rejected,
          "The restored Shader is missing one required Stage Function."_view);
    }
    if (!callable_interface.accepts(*required, *supplied)) {
      return Evaluation(
          Tetrodotoxin::Source::Interface::Relation::Rejected,
          "The restored Stage Function no longer has a compatible Signature."_view);
    }
    View::Bytes parameter_failure = slot_failure(
        supplied->get_signature().get_parameters(),
        required->get_parameter_layout());
    if (!parameter_failure.is_empty()) {
      return Evaluation(
          Tetrodotoxin::Source::Interface::Relation::Rejected, parameter_failure);
    }
    View::Bytes result_failure = slot_failure(
        supplied->get_signature().get_results(), required->get_result_layout());
    if (!result_failure.is_empty()) {
      return Evaluation(
          Tetrodotoxin::Source::Interface::Relation::Rejected, result_failure);
    }
  }

  // A shared Type keeps exact identity. A Render Structure and its Shader
  // implementation remain distinct Types, so this concrete Contract proves
  // their named binding and mutually fitting Layout together.
  for (const Reference<Abstract>& entry : render->get_addressables()) {
    auto required = entry.get().select<Render::Language::Binding>();
    if (!required) {
      return Evaluation(
          Tetrodotoxin::Source::Interface::Relation::Rejected,
          "The restored Pipeline value is not one Binding."_view);
    }
    auto supplied = find_binding(*shader, required->get_name());
    if (!supplied) {
      return Evaluation(
          Tetrodotoxin::Source::Interface::Relation::Rejected,
          "The restored Shader is missing one required Binding."_view);
    }
    const Library::Language::Field& field = supplied->get_field();
    if (supplied->get_kind() != required->get_kind() ||
        supplied->get_access() != required->get_access() ||
        !Evaluation::compatible_binding_type(
            required->get_type(), field.get_type())) {
      return Evaluation(
          Tetrodotoxin::Source::Interface::Relation::Rejected,
          "The restored Shader Binding no longer has its required kind and Type."_view);
    }
  }
  return Evaluation(Tetrodotoxin::Source::Interface::Relation::Satisfied);
}

auto Shader::Language::Contract::negotiate(
    const Abstract& requirement,
    const Abstract& candidate) const -> Relation {
  return evaluate(requirement, candidate).relation;
}

auto Shader::Language::Contract::validate(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Abstract& requirement,
    const Abstract& candidate) const -> Bool {
  Relation relation = negotiate(requirement, candidate);
  if (relation != Relation::Rejected) {
    return True;
  }

  auto program = candidate.select<Shader::Language::Program>();
  cursor.create_expression_error(
      program ? program->get_anchor()
              : Tetrodotoxin::Source::Lexical::Anchor::create(Tetrodotoxin::Source::Lexical::Span()),
      "Shader does not satisfy its selected Pipeline contract."_view,
      "Match every required Stage, value, and Type."_view);
  return False;
}

auto Shader::Language::Contract::validate_restored(
    const Abstract& requirement,
    const Abstract& candidate) const -> Bool {
  Evaluation evaluation = evaluate(requirement, candidate);
  if (evaluation.relation != Relation::Rejected) {
    return True;
  }
  Diagnostics::Log::error(evaluation.failure);
  return False;
}
