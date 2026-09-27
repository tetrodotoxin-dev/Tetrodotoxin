// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/monograph.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

static constexpr Tetrodotoxin::Source::Layouts::Fluid monograph_layout;

Language::Monograph::~Monograph() {}

Language::Monograph::Monograph(
    Allocator::Arena& domain,
    const Abstract& language,
    const Tetrodotoxin::Source::Documentation& documentation,
    Abstract& context)
    : domain(domain),
      documentation(documentation),
      context(context),
      language(language),
      imports(domain) {}

auto Language::Monograph::get_layer(const Abstract& requested) const
    -> Option<const Monograph&> {
  if (&requested == &language) {
    return *this;
  }

  return {};
}

auto Language::Monograph::get_root() const -> const Abstract& {
  return *this;
}

auto Language::Monograph::retain_import(
    const Import::Description& description,
    Option<Associations&> associations) -> Bool {
  for (const Reference<Import>& import : imports.get_view()) {
    BAIL_IF(import.get().get_name() == description.get_name());
  }

  Import& import = domain.construct<Import>(domain, description);
  imports.insert(import);
  if (associations) {
    associations->create(description.get_declaration_anchor(), import);
    associations->create(description.get_expression_anchor(), import);
  }
  return True;
}

auto Language::Monograph::compose(Cursor&) -> Bool {
  return True;
}

auto Language::Monograph::link(Cursor&) -> Bool {
  return True;
}

auto Language::Monograph::finalize(Cursor&) -> Bool {
  return True;
}

auto Language::Monograph::compose_restored() -> Bool {
  return True;
}

auto Language::Monograph::link_restored() -> Bool {
  return True;
}

auto Language::Monograph::finalize_restored() -> Bool {
  return True;
}

auto Language::Monograph::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  // A base Monograph contributes no synthetic lookup surface. Concrete roots
  // answer their own names first and use this boundary only for the borrowed
  // outer context supplied by the source transaction.
  const Abstract& imported = resolve_type(route, Visibility::Public);
  return imported.is<Unknown>() ? context.resolve_concept(route) : imported;
}

auto Language::Monograph::resolve_lexical_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& imported = resolve_type(route, Visibility::Private);
  return imported.is<Unknown>() ? context.resolve_concept(route) : imported;
}

auto Language::Monograph::resolve_type(View::Bytes route, Visibility visibility)
    const -> const Abstract& {
  for (const Reference<Import>& retained : imports.get_view()) {
    const Import& selected = retained.get();
    if (selected.get_name() != route) {
      continue;
    }

    if (visibility == Visibility::Private ||
        selected.get_visibility() != Visibility::Private) {
      return selected;
    }
  }

  return Unknown::get_unknown();
}

auto Language::Monograph::get_layout() const -> const Layout& {
  return monograph_layout;
}
