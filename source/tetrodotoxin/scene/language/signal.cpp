// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/language/signal.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

auto Scene::Language::Signal::create_authored(
    Allocator::Arena& domain,
    const Tetrodotoxin::Source::Documentation& documentation,
    View::Bytes name,
    Token name_token,
    Option<Library::Language::TypeReference> payload,
    Anchor anchor) -> Signal& {
  return domain.construct_from<Signal>([&]() -> Signal {
    return Signal(documentation, name, name_token, payload, anchor);
  });
}

auto Scene::Language::Signal::create_restored(
    Allocator::Arena& domain,
    const Tetrodotoxin::Source::Documentation& documentation,
    View::Bytes name,
    Option<Library::Language::TypeReference> payload) -> Signal& {
  return domain.construct_from<Signal>([&]() -> Signal {
    return Signal(
        documentation, domain.proxy(name), {}, payload, Anchor::create(Span()));
  });
}

auto Scene::Language::Signal::link(Cursor& cursor, const Abstract& context)
    -> Bool {
  if (linked) {
    return True;
  }

  if (payload) {
    auto selected = payload->resolve_authored(cursor, context);
    auto type =
        selected ? selected->resolve().select<Library::Language::Model::Type>()
                 : Option<const Library::Language::Model::Type&>();
    if (!type) {
      cursor.create_expression_error(
          payload->get_anchor(),
          "Scene Signal payload requires one completed Library Type."_view);
      return False;
    }
    payload_type = *type;
  }

  if (name_token) {
    cursor.get_associations().create(
        Anchor::create(name_token, Span(name_token)), *this);
  }
  linked = True;
  return True;
}

auto Scene::Language::Signal::link_restored(const Abstract& context) -> Bool {
  if (linked) {
    return True;
  }

  if (payload) {
    Option<const Library::Language::Model::Type&> type;
    payload->resolve(context).visit(
        [&](const Abstract& selected) {
          type = selected.resolve().select<Library::Language::Model::Type>();
        },
        [](const Library::Language::TypeReference::Failure&) {});
    BAIL_IF(!type);
    payload_type = *type;
  }
  linked = True;
  return True;
}

auto Scene::Language::Signal::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  return Abstract::resolve_concept(route);
}
