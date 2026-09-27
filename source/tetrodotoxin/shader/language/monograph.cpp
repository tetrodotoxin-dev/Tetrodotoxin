// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/language/monograph.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

auto Shader::Language::Monograph::create(
    Allocator::Arena& domain,
    const Abstract& language,
    const Tetrodotoxin::Source::Documentation& documentation,
    Abstract& context,
    Library::Language::Monograph& library) -> Monograph& {
  return domain.construct_from<Monograph>([&]() {
    return Monograph(domain, language, documentation, context, library);
  });
}

auto Shader::Language::Monograph::retain_program(Program& program) -> Bool {
  for (const Reference<Program>& retained : programs.get_view()) {
    BAIL_IF(retained.get().get_name() == program.get_name());
  }
  programs.insert(program);
  return True;
}

auto Shader::Language::Monograph::retain_bridge(Bridge& bridge) -> Bool {
  for (const Reference<Bridge>& retained : bridges.get_view()) {
    BAIL_IF(retained.get().get_name() == bridge.get_name());
  }
  bridges.insert(bridge);
  return True;
}

auto Shader::Language::Monograph::compose(Cursor& cursor) -> Bool {
  Bool valid = True;
  for (Reference<Program> program : programs.get_view()) {
    valid &= program.get().compose_contract(cursor);
  }
  return valid;
}

auto Shader::Language::Monograph::link(Cursor& cursor) -> Bool {
  if (linked) {
    return True;
  }
  Bool valid = library.link(cursor);
  for (Reference<Bridge> bridge : bridges.get_view()) {
    valid &= bridge.get().link(cursor, *this);
  }
  linked = valid;
  return valid;
}

auto Shader::Language::Monograph::finalize(Cursor& cursor) -> Bool {
  if (finalized) {
    return True;
  }
  Bool valid = library.finalize(cursor);
  for (Reference<Program> program : programs.get_view()) {
    valid &= program.get().validate_contract(cursor);
  }
  finalized = valid;
  return valid;
}

auto Shader::Language::Monograph::compose_restored() -> Bool {
  Bool valid = True;
  for (Reference<Program> program : programs.get_view()) {
    valid &= program.get().compose_contract_restored();
  }
  return valid;
}

auto Shader::Language::Monograph::link_restored() -> Bool {
  if (linked) {
    return True;
  }

  Bool valid = library.link_restored();
  for (Reference<Bridge> bridge : bridges.get_view()) {
    valid &= bridge.get().link_restored(*this);
  }
  linked = valid;
  return valid;
}

auto Shader::Language::Monograph::finalize_restored() -> Bool {
  if (finalized) {
    return True;
  }

  Bool valid = library.finalize_restored();
  if (!valid) {
    Diagnostics::Log::error(
        "Shader Archive could not finalize its restored Library child."_view);
  }
  for (Reference<Program> program : programs.get_view()) {
    if (!program.get().validate_contract_restored()) {
      Diagnostics::Log::Message<256> message(
          Diagnostics::Log::Level::Error, Diagnostics::Source());
      message << "Shader Archive Program `"_view << program.get().get_name()
              << "` no longer satisfies its restored Pipeline contract."_view;
      valid = False;
    }
  }
  finalized = valid;
  return valid;
}

auto Shader::Language::Monograph::get_layer(const Abstract& requested) const
    -> Option<const Tetrodotoxin::Language::Monograph&> {
  auto outer = Tetrodotoxin::Language::Monograph::get_layer(requested);
  if (outer) {
    return *outer;
  }
  return &requested == &library.get_language()
             ? Option<const Tetrodotoxin::Language::Monograph&>(library)
             : Option<const Tetrodotoxin::Language::Monograph&>();
}

static auto find_shader_member(const Shader::Language::Monograph& owner,
                               View::Bytes route) -> Option<const Abstract&> {
  for (const Reference<Shader::Language::Program>& program :
       owner.get_programs()) {
    if (route == "Material"_view) {
      return program.get().get_instance();
    }
  }
  for (const Reference<Shader::Language::Bridge>& bridge :
       owner.get_bridges()) {
    if (bridge.get().get_name() == route) {
      return bridge.get();
    }
  }
  return {};
}

auto Shader::Language::Monograph::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  if (route == "static"_view) {
    return static_scope;
  }
  return static_scope.resolve_concept(route);
}

auto Shader::Language::Monograph::Authority::resolve_concept(
    View::Bytes route) const -> const Abstract& {
  if (route == "static"_view || route == "instance"_view) {
    return None::get_none();
  }
  auto local = find_shader_member(owner, route);
  if (local) {
    return *local;
  }
  const Abstract& child = owner.library.resolve_concept(route);
  return child.is<Unknown>() || child.is<None>()
             ? owner.Tetrodotoxin::Language::Monograph::resolve_concept(route)
             : child;
}

auto Shader::Language::Monograph::visit_concepts(
    Abstract::Visitor visitor) const -> void {
  visitor("static"_view, static_scope);
}

auto Shader::Language::Monograph::Authority::visit_concepts(
    Abstract::Visitor visitor) const -> void {
  if (!owner.programs.is_empty()) {
    visitor("Material"_view, owner.programs.at(0).get().get_instance());
  }
  for (const Reference<Bridge>& retained : owner.bridges.get_view()) {
    const Bridge& bridge = retained.get();
    const auto name = bridge.get_name();
    if (&resolve_concept(name) == &bridge) {
      visitor(name, bridge);
    }
  }

  // Resolve the child's advertised names through Shader's overlay. Local
  // members were already offered above, and only unshadowed Library answers
  // belong to the remaining public surface.
  auto receive = [&](View::Bytes name, const Abstract& candidate) {
    if (!find_shader_member(owner, name) &&
        &resolve_concept(name) == &candidate) {
      visitor(name, candidate);
    }
  };
  owner.library.resolve_concept("static"_view)
      .visit_concepts(Abstract::Visitor(receive));
}

auto Shader::Language::Monograph::resolve_lexical_context(
    View::Bytes route) const -> const Abstract& {
  auto local = find_shader_member(*this, route);
  if (local) {
    return *local;
  }

  const Abstract& child = library.resolve_lexical_context(route);
  return child.is<Unknown>() || child.is<None>()
             ? Tetrodotoxin::Language::Monograph::resolve_lexical_context(route)
             : child;
}
