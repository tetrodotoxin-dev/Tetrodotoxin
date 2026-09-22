// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/language/monograph.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

static auto is_resource_route(View::Bytes route) -> Bool {
  return route.get_size() >= 3 && route[0] == '$' && route[1] == '[' &&
         route[route.get_size() - 1] == ']';
}

auto Package::Language::Monograph::create_authored(
    Allocator::Arena& arena,
    const Abstract& language,
    const Tetrodotoxin::Source::Documentation& documentation,
    const Anchor& source_anchor,
    View::Bytes identity,
    Perimortem::System::Version version,
    Abstract& context,
    const Abstract& library_language) -> Monograph& {
  return arena.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(
        arena, language, documentation, source_anchor, identity, version,
        context, library_language, {}, False);
  });
}

auto Package::Language::Monograph::create_synthetic(
    Allocator::Arena& arena,
    const Abstract& language,
    View::Bytes identity,
    Perimortem::System::Version version,
    Abstract& context,
    const Abstract& library_language,
    View::Vector<Reference<Package::Resource>> restored_resources)
    -> Monograph& {
  Monograph& monograph = arena.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(
        arena, language, Tetrodotoxin::Source::Documentation::get_empty(), Anchor::create(Span()),
        identity, version, context, library_language, restored_resources, True);
  });
  return monograph;
}

Package::Language::Monograph::Monograph(
    Allocator::Arena& arena,
    const Abstract& language,
    const Tetrodotoxin::Source::Documentation& documentation,
    const Anchor& source_anchor,
    View::Bytes identity,
    Perimortem::System::Version version,
    Abstract& context,
    const Abstract& library_language,
    View::Vector<Reference<Package::Resource>> restored_resources,
    Bool resources_sealed)
    : Tetrodotoxin::Language::Monograph(
          arena,
          language,
          documentation,
          context),
      resources(domain, restored_resources, resources_sealed),
      identity(identity),
      version(version),
      library(
          Library::Language::Monograph::create_authored(
              domain,
              documentation,
              source_anchor,
              library_language,
              *this)) {}

auto Package::Language::Monograph::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  // The delimiters reserve one complete resource route. Malformed or partial
  // spellings continue through exact Package lookup so this branch never
  // becomes a second Embedded parser.
  if (is_resource_route(route)) {
    return resources.resolve(route.slice(2, route.get_size() - 3));
  }
  if (route == "static"_view) {
    return library.get_source();
  }
  if (route == "instance"_view) {
    return None::get_none();
  }

  return library.get_source().resolve_public_context(route);
}

auto Package::Language::Monograph::visit_concepts(
    Tetrodotoxin::Source::Abstract::Visitor visitor) const -> void {
  visitor("static"_view, library.get_source());
}

auto Package::Language::Monograph::retain_import(
    const Tetrodotoxin::Language::Import::Description& description,
    Option<Associations&> associations) -> Bool {
  BAIL_IF(!library.retain_import(description, associations));
  Tetrodotoxin::Language::Import& import =
      library.get_imports()
          .get_data()[library.get_imports().get_size() - 1]
          .get();
  Bool published = description.get_visibility() !=
                   Tetrodotoxin::Language::Visibility::Private;
  return library.get_source().bind_static(
      import, Library::Language::Types::Composite::Category::Type, published);
}

auto Package::Language::Monograph::resolve_lexical_context(
    View::Bytes route) const -> const Abstract& {
  if (is_resource_route(route)) {
    return resources.resolve(route.slice(2, route.get_size() - 3));
  }

  const Abstract& selected = library.resolve_lexical_context(route);
  return selected;
}

auto Package::Language::Monograph::get_layer(const Abstract& requested) const
    -> Option<const Tetrodotoxin::Language::Monograph&> {
  auto outer = Tetrodotoxin::Language::Monograph::get_layer(requested);
  if (outer) {
    return *outer;
  }
  return &requested == &library.get_language()
             ? Option<const Tetrodotoxin::Language::Monograph&>(library)
             : Option<const Tetrodotoxin::Language::Monograph&>();
}

auto Package::Language::Monograph::link(Cursor& cursor) -> Bool {
  return library.link(cursor);
}

auto Package::Language::Monograph::finalize(Cursor& cursor) -> Bool {
  BAIL_IF(!library.finalize(cursor));
  return True;
}

auto Package::Language::Monograph::link_restored() -> Bool {
  return library.link_restored();
}

auto Package::Language::Monograph::finalize_restored() -> Bool {
  BAIL_IF(!library.finalize_restored());
  return True;
}

auto Package::Language::Monograph::get_name() const -> View::Bytes {
  return identity;
}

auto Package::Language::Monograph::get_resources() -> Package::Resources& {
  return resources;
}

auto Package::Language::Monograph::get_resources() const
    -> const Package::Resources& {
  return resources;
}
