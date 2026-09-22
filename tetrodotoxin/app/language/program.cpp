// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/language/program.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::App;

static auto resolve_route(const Abstract& context, View::Bytes route)
    -> const Abstract& {
  Reference<const Abstract> selected(context);
  Count start = 0;
  for (Count index = 0; index <= route.get_size(); index++) {
    Bool terminal = index == route.get_size();
    Bool separator = !terminal && index + 1 < route.get_size() &&
                     route[index] == ':' && route[index + 1] == ':';
    if (!terminal && !separator) {
      continue;
    }

    View::Bytes segment = route.slice(start, index - start);
    if (segment.is_empty()) {
      return Unknown::get_unknown();
    }

    const Abstract& queried =
        selected.get().visit<Tetrodotoxin::Language::Monograph>(
            [&](const Tetrodotoxin::Language::Monograph& monograph)
                -> const Abstract& {
              return start == 0 ? monograph.resolve_lexical_context(segment)
                                : monograph.resolve_concept(segment);
            },
            [&](const Abstract& selected_context) -> const Abstract& {
              return selected_context.resolve_concept(segment);
            });
    const Abstract& candidate = queried.resolve();
    if (candidate.is<Unknown>() || candidate.is<None>()) {
      return candidate;
    }
    selected = Reference<const Abstract>(candidate);

    if (separator) {
      index++;
      start = index + 1;
    }
  }

  return selected.get();
}

auto Language::Program::create_authored(
    Perimortem::Memory::Allocator::Arena& arena,
    const Tetrodotoxin::Source::Documentation& documentation,
    View::Bytes route,
    View::Bytes callable_name,
    Anchor anchor,
    Anchor selection_anchor) -> Program& {
  return arena.construct_from<Program>([&]() {
    return Program(
        documentation, route, callable_name, anchor, selection_anchor);
  });
}

auto Language::Program::link(Cursor& cursor, Abstract& context) -> Bool {
  const Abstract& receiver = resolve_route(context, route);
  if (receiver.is<Unknown>() || receiver.is<None>()) {
    auto report = cursor.create_report(selection_anchor);
    report << "Program entry route `"_view << route
           << "` does not resolve in this Package."_view;
    report.get_hint()
        << "Select one exact Package member or nested public context."_view;
    return False;
  }

  const Abstract& selected = receiver.resolve_concept("static"_view)
                                 .resolve_concept(callable_name)
                                 .resolve();
  auto callable = selected.select<Tetrodotoxin::Source::Callable>();
  if (!callable) {
    auto report = cursor.create_report(selection_anchor);
    report << "Program entry `"_view << route << " -> "_view << callable_name
           << "` does not select a Static Callable."_view;
    report.get_hint()
        << "Publish one Callable with empty parameters and results."_view;
    return False;
  }

  if (!callable->get_parameters().is_empty() ||
      !callable->get_results().is_empty()) {
    auto report = cursor.create_report(selection_anchor);
    report << "Program entry Callable `"_view << callable_name
           << "` must have empty parameter and result Layouts."_view;
    report.get_hint() << "Use a Static `[] -> []` Callable."_view;
    return False;
  }

  entry = Reference<const Tetrodotoxin::Source::Callable>(*callable);
  cursor.get_associations().create(selection_anchor, *callable);
  return True;
}

auto Language::Program::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  return Abstract::resolve_concept(route);
}
