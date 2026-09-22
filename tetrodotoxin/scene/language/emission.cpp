// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/language/emission.hpp"

#include "tetrodotoxin/scene/language/monograph.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto Scene::Language::Emission::create_authored(
    Allocator::Arena& domain,
    Abstract& scene,
    View::Bytes signal_name,
    Token signal_token,
    Option<Library::Language::Model::Pack&> payload,
    Anchor anchor) -> Emission& {
  return domain.construct_from<Emission>([&]() -> Emission {
    return Emission(scene, signal_name, signal_token, payload, anchor);
  });
}

auto Scene::Language::Emission::link(
    Cursor& cursor,
    Library::Language::Flow::Scope& scope) -> Bool {
  if (!signal) {
    auto monograph = scene.select<Scene::Language::Monograph>();
    BAIL_IF(!monograph);
    auto selected = monograph->find_signal(signal_name);
    if (!selected) {
      cursor.create_token_error(
          signal_token, "Scene emission names an unknown Signal."_view,
          "Declare the Signal on this Scene before emitting it."_view);
      return False;
    }
    signal = *selected;
    cursor.get_associations().create(
        Anchor::create(signal_token, Span(signal_token)), *signal);
  }

  return !payload || payload->link(cursor, scope, scope.get_access_scope());
}

auto Scene::Language::Emission::validate(Cursor& cursor) const -> Bool {
  BAIL_IF(!signal || !signal->is_linked());

  auto payload_type = signal->get_payload_type();
  if (!payload_type && !payload) {
    return True;
  }
  if (!payload_type) {
    cursor.create_expression_error(
        anchor, "This Scene Signal does not carry a value."_view,
        "Emit it without an argument list."_view);
    return False;
  }
  if (!payload) {
    cursor.create_expression_error(
        anchor, "This Scene Signal requires one value."_view,
        "Supply a value that fits the Signal payload Type."_view);
    return False;
  }
  if (!payload->fits_into(*payload_type)) {
    cursor.create_expression_error(
        anchor,
        "Scene emission value does not fit its Signal payload Type."_view);
    return False;
  }
  return True;
}

auto Scene::Language::Emission::finalize(Cursor& cursor) -> void {
  if (payload) {
    payload->finalize(cursor);
  }
}

auto Scene::Language::Emission::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  return Abstract::resolve_concept(route);
}
