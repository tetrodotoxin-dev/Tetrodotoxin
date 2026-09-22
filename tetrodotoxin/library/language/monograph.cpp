// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/monograph.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/library/language/generics/access.hpp"
#include "tetrodotoxin/library/language/generics/fixed.hpp"
#include "tetrodotoxin/library/language/generics/implementation.hpp"
#include "tetrodotoxin/library/language/generics/object.hpp"
#include "tetrodotoxin/library/language/generics/option.hpp"
#include "tetrodotoxin/library/language/generics/range.hpp"
#include "tetrodotoxin/library/language/generics/result.hpp"
#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/r32.hpp"
#include "tetrodotoxin/library/language/types/r64.hpp"
#include "tetrodotoxin/library/language/types/s16.hpp"
#include "tetrodotoxin/library/language/types/s32.hpp"
#include "tetrodotoxin/library/language/types/s64.hpp"
#include "tetrodotoxin/library/language/types/s8.hpp"
#include "tetrodotoxin/library/language/types/u16.hpp"
#include "tetrodotoxin/library/language/types/u32.hpp"
#include "tetrodotoxin/library/language/types/u64.hpp"
#include "tetrodotoxin/library/language/types/u8.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

static auto is_missing(const Abstract& abstract) -> Bool {
  return abstract.is<Unknown>() || abstract.is<None>();
}

Library::Language::Monograph::Monograph(
    Allocator::Arena& arena,
    const Tetrodotoxin::Source::Documentation& documentation,
    const Anchor& source_anchor,
    const Abstract& language,
    Abstract& context)
    : Tetrodotoxin::Language::Monograph(
          arena,
          language,
          documentation,
          context),
      vocabulary(domain),
      source(
          Types::Source::create_synthetic(
              domain,
              documentation,
              *this,
              source_anchor)) {
  // Each Library root owns the identities that give its source meaning. Keeping
  // the vocabulary beside the graph makes lookup and materialization observe
  // the same objects without a second Type inventory.
  Abstract* identities[] = {
    &domain.construct<Types::Boolean>(),
    &domain.construct<Types::U8>(),
    &domain.construct<Types::U16>(),
    &domain.construct<Types::U32>(),
    &domain.construct<Types::U64>(),
    &domain.construct<Types::S8>(),
    &domain.construct<Types::S16>(),
    &domain.construct<Types::S32>(),
    &domain.construct<Types::S64>(),
    &domain.construct<Types::R32>(),
    &domain.construct<Types::R64>(),
    &domain.construct<Generics::Access>(domain, *this),
    &domain.construct<Generics::Fixed>(domain, *this),
    &domain.construct<Generics::Implementation>(domain, *this),
    &domain.construct<Generics::Option>(domain, *this),
    &domain.construct<Generics::Object>(domain, *this),
    &domain.construct<Generics::Range>(domain, *this),
    &domain.construct<Generics::Result>(domain, *this),
    &domain.construct<Generics::View>(domain, *this),
  };
  for (Abstract* identity : identities) {
    vocabulary.launder(identity->get_name(), *identity);
  }
}

auto Library::Language::Monograph::create_authored(
    Allocator::Arena& arena,
    const Tetrodotoxin::Source::Documentation& documentation,
    const Anchor& source_anchor,
    const Abstract& language,
    Abstract& context) -> Monograph& {
  return create(arena, documentation, source_anchor, language, context);
}

auto Library::Language::Monograph::create(
    Allocator::Arena& arena,
    const Tetrodotoxin::Source::Documentation& documentation,
    const Anchor& source_anchor,
    const Abstract& language,
    Abstract& context) -> Monograph& {
  return arena.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(arena, documentation, source_anchor, language, context);
  });
}

auto Library::Language::Monograph::link(Cursor& cursor) -> Bool {
  return source.link(cursor, *this);
}

auto Library::Language::Monograph::finalize(Cursor& cursor) -> Bool {
  Bool valid = True;
  for (Count index = 0; index < vocabulary.get_size(); index++) {
    auto entry = vocabulary.get_entry(index);
    Option<Generic&> generic;
    if (entry) {
      generic = entry->value.select<Generic>();
    }
    if (generic) {
      valid &= generic->validate_materializations(cursor);
    }
  }

  return valid && source.finalize(cursor);
}

auto Library::Language::Monograph::link_restored() -> Bool {
  return source.link_restored(*this);
}

auto Library::Language::Monograph::finalize_restored() -> Bool {
  return source.finalize_restored();
}

auto Library::Language::Monograph::get_name() const -> View::Bytes {
  return "Library"_view;
}

auto Library::Language::Monograph::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  if (route == "static"_view || route == "instance"_view) {
    return source.resolve_concept(route);
  }
  const Abstract& local = resolve_local_context(route);
  return is_missing(local)
             ? Tetrodotoxin::Language::Monograph::resolve_concept(route)
             : local;
}

auto Library::Language::Monograph::visit_concepts(
    Tetrodotoxin::Source::Abstract::Visitor visitor) const -> void {
  source.visit_concepts(visitor);
}

auto Library::Language::Monograph::resolve_local_context(
    View::Bytes route) const -> const Abstract& {
  // Source and Foreign are the two reserved authored contexts. Ordinary
  // declarations remain in Source while root vocabulary and using contexts
  // answer only names that those owned contexts leave unresolved.
  if (route == "source"_view) {
    return source;
  }

  if (route == "foreign"_view && source.get_foreign().is_authored()) {
    return source.get_foreign();
  }

  const Abstract& authored = source.resolve_local(route);
  if (!is_missing(authored)) {
    return authored;
  }

  const Abstract& root = resolve_root_context(route);
  if (!is_missing(root)) {
    return root;
  }

  const Abstract& imported = source.resolve_imports(route);
  return is_missing(imported)
             ? resolve_type(route, Tetrodotoxin::Language::Visibility::Public)
             : imported;
}

auto Library::Language::Monograph::resolve_lexical_context(
    View::Bytes route) const -> const Abstract& {
  if (route == "source"_view) {
    return source;
  }

  if (route == "foreign"_view && source.get_foreign().is_authored()) {
    return source.get_foreign();
  }

  const Abstract& authored =
      source.resolve_local(route, Tetrodotoxin::Language::Visibility::Private);
  if (!is_missing(authored)) {
    return authored;
  }

  const Abstract& root = resolve_root_context(route);
  if (!is_missing(root)) {
    return root;
  }

  const Abstract& imported = source.resolve_imports(route);
  return is_missing(imported)
             ? Tetrodotoxin::Language::Monograph::resolve_lexical_context(route)
             : imported;
}

auto Library::Language::Monograph::can_bind_source_type(View::Bytes name) const
    -> Bool {
  return resolve_root_context(name).is<Unknown>() &&
         resolve_type(name, Tetrodotoxin::Language::Visibility::Private)
             .is<Unknown>();
}

auto Library::Language::Monograph::retain_import(
    const Tetrodotoxin::Language::Import::Description& description,
    Option<Associations&> associations) -> Bool {
  BAIL_IF(!can_bind_source_type(description.get_name()));
  return Tetrodotoxin::Language::Monograph::retain_import(
      description, associations);
}

auto Library::Language::Monograph::resolve_root_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& intrinsic = vocabulary.visit(
      route,
      [](const Abstract& selected) -> const Abstract& { return selected; },
      []() -> const Abstract& { return Unknown::get_unknown(); });
  if (!intrinsic.is<Unknown>()) {
    return intrinsic;
  }

  return Unknown::get_unknown();
}
