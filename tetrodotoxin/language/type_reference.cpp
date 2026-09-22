// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/type_reference.hpp"

#include "tetrodotoxin/language/import.hpp"
#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

static auto is_missing(const Abstract& abstract) -> Bool {
  return abstract.is<Unknown>() || abstract.is<None>();
}

// A native Type identity can be useful before its full resolve answer becomes
// factual. Import supplies that Type through its own operation, while other
// declarations and transparent references follow ordinary resolution.
static auto select_native(const Abstract& candidate) -> const Abstract& {
  if (candidate.is<Tetrodotoxin::Source::Type>()) {
    return candidate;
  }
  const Abstract& resolved = candidate.resolve();
  return resolved.is<Tetrodotoxin::Language::Import>() ? resolved.get_type()
                                                       : resolved;
}

static auto segment_anchor(
    Anchor route_anchor,
    View::Bytes route,
    View::Bytes name) -> Anchor {
  Token first = route_anchor.get_token();
  if (!first || name.is_empty()) {
    return Anchor::create(Span());
  }

  Count offset = Count(name.get_data() - route.get_data());
  Token token(
      U16(Count(first.get_offset()) + offset), first.get_line(),
      U16(Count(first.get_column()) + offset), U8(name.get_size()),
      first.get_code());
  return Anchor::create(token, Span(token));
}

static auto resolve_route(
    View::Bytes route,
    const Abstract& context,
    Option<Cursor&> cursor,
    Anchor anchor,
    Option<const Abstract&> supplied_root = {})
    -> Option<const Tetrodotoxin::Source::Type&> {
  const Abstract* selected = &context;
  Count start = 0;
  Count segment = 0;
  for (Count index = 0; index <= route.get_size(); index++) {
    Bool terminal = index == route.get_size();
    Bool separator = !terminal && index + 1 < route.get_size() &&
                     route[index] == ':' && route[index + 1] == ':';
    if (!terminal && !separator) {
      continue;
    }

    View::Bytes name = route.slice(start, index - start);
    const Abstract* queried = nullptr;
    if (segment == 0 && supplied_root) {
      queried = &*supplied_root;
    } else if (segment == 0) {
      queried = &context.visit<Language::Monograph>(
          [&](const Language::Monograph& monograph) -> const Abstract& {
            return monograph.resolve_lexical_context(name);
          },
          [&](const Abstract&) -> const Abstract& {
            return selected->resolve_concept(name);
          });
    } else {
      queried = &selected->resolve_concept(name);
    }

    if (is_missing(*queried)) {
      if (cursor) {
        auto report = cursor->create_report(anchor);
        report << "Type route `"_view << route
               << "` could not resolve segment "_view << U64(segment)
               << "."_view;
        report.get_hint()
            << "Publish that Type in the selected semantic context."_view;
      }
      return {};
    }
    if (cursor && !terminal) {
      cursor->get_associations().create(
          segment_anchor(anchor, route, name), *queried);
    }
    selected = queried;
    segment++;

    if (separator) {
      index++;
      start = index + 1;
    }
  }

  View::Bytes terminal_name;
  Count terminal_start = 0;
  for (Count index = 0; index <= route.get_size(); index++) {
    Bool terminal = index == route.get_size();
    Bool separator = !terminal && index + 1 < route.get_size() &&
                     route[index] == ':' && route[index + 1] == ':';
    if (separator) {
      index++;
      terminal_start = index + 1;
    } else if (terminal) {
      terminal_name = route.slice(terminal_start, index - terminal_start);
    }
  }
  const Abstract& resolved = select_native(*selected);
  auto type = resolved.select<Tetrodotoxin::Source::Type>();
  if (!type) {
    if (cursor) {
      auto report = cursor->create_report(anchor);
      report << "Route `"_view << route << "` does not select one Type."_view;
      report.get_hint()
          << "Select one value Type rather than a contextual declaration."_view;
    }
    return {};
  }

  if (cursor) {
    cursor->get_associations().create(
        segment_anchor(anchor, route, terminal_name), *type);
  }
  return *type;
}

auto Language::TypeReference::resolve(Cursor& cursor, const Abstract& context)
    const -> Option<const Tetrodotoxin::Source::Type&> {
  return resolve_route(route, context, cursor, anchor);
}

auto Language::TypeReference::resolve_selected(
    Cursor& cursor,
    const Abstract& selected_root) const -> Option<const Tetrodotoxin::Source::Type&> {
  return resolve_route(route, selected_root, cursor, anchor, selected_root);
}

auto Language::TypeReference::resolve_restored(const Abstract& context) const
    -> Option<const Tetrodotoxin::Source::Type&> {
  return resolve_route(route, context, {}, anchor);
}

auto Language::TypeReference::resolve_restored_selected(
    const Abstract& selected_root) const -> Option<const Tetrodotoxin::Source::Type&> {
  return resolve_route(route, selected_root, {}, anchor, selected_root);
}

auto Language::TypeReference::get_root() const -> View::Bytes {
  for (Count index = 0; index + 1 < route.get_size(); index++) {
    if (route[index] == ':' && route[index + 1] == ':') {
      return route.slice(0, index);
    }
  }

  return route;
}
