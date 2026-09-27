// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/monograph.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Render;

auto Language::Monograph::create(
    Allocator::Arena& arena,
    const Abstract& language,
    const Tetrodotoxin::Source::Documentation& documentation,
    Abstract& context) -> Monograph& {
  return arena.construct_from<Monograph>(
      [&]() { return Monograph(arena, language, documentation, context); });
}

auto Language::Monograph::retain_addressable(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  return declarations.retain_addressable(declaration, visibility);
}

auto Language::Monograph::retain_callable(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  return declarations.retain_callable(declaration, visibility);
}

auto Language::Monograph::retain_type(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  return declarations.retain_type(declaration, visibility);
}

auto Language::Monograph::retain_import(
    const Tetrodotoxin::Language::Import::Description& description,
    Option<Associations&> associations) -> Bool {
  BAIL_IF(!declarations
               .resolve_type(
                   description.get_name(),
                   Tetrodotoxin::Language::Visibility::Private)
               .is<Unknown>());
  return Tetrodotoxin::Language::Monograph::retain_import(
      description, associations);
}

auto Language::Monograph::compose(Cursor& cursor) -> Bool {
  return declarations.link(cursor, *this);
}

auto Language::Monograph::link(Cursor& cursor) -> Bool {
  return declarations.link(cursor, *this);
}

auto Language::Monograph::finalize(Cursor&) -> Bool {
  finalized = declarations.is_linked();
  return finalized;
}

auto Language::Monograph::link_restored() -> Bool {
  return declarations.link_restored(*this);
}

auto Language::Monograph::compose_restored() -> Bool {
  return declarations.link_restored(*this);
}

auto Language::Monograph::finalize_restored() -> Bool {
  finalized = declarations.is_linked();
  return finalized;
}

auto Language::Monograph::resolve_concept(View::Bytes name) const
    -> const Abstract& {
  if (name == "static"_view) {
    return declarations.get_authority();
  }
  if (name == "instance"_view) {
    return None::get_none();
  }
  const Abstract& local = declarations.resolve_type(
      name, Tetrodotoxin::Language::Visibility::Public);
  return local.is<Unknown>() || local.is<None>()
             ? Tetrodotoxin::Language::Monograph::resolve_concept(name)
             : local;
}

auto Language::Monograph::visit_concepts(
    Tetrodotoxin::Source::Abstract::Visitor visitor) const -> void {
  declarations.visit_concepts(visitor);
}

auto Language::Monograph::resolve_lexical_context(View::Bytes name) const
    -> const Abstract& {
  const Abstract& local = declarations.resolve_type(
      name, Tetrodotoxin::Language::Visibility::Private);
  return local.is<Unknown>() || local.is<None>()
             ? Tetrodotoxin::Language::Monograph::resolve_lexical_context(name)
             : local;
}
