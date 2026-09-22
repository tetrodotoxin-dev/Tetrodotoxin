// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/language/route.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;

static auto resolve_route(View::Bytes spelling, const Abstract& context)
    -> Option<const Abstract&> {
  Reference<const Abstract> selected(context);
  Count start = 0;
  for (Count index = 0; index <= spelling.get_size(); index++) {
    Bool terminal = index == spelling.get_size();
    Bool separator = !terminal && index + 1 < spelling.get_size() &&
                     spelling[index] == ':' && spelling[index + 1] == ':';
    if (!terminal && !separator) {
      continue;
    }

    View::Bytes segment = spelling.slice(start, index - start);
    BAIL_IF(segment.is_empty());
    const Abstract& queried = selected.get().visit<Language::Monograph>(
        [&](const Language::Monograph& monograph) -> const Abstract& {
          return start == 0 ? monograph.resolve_lexical_context(segment)
                            : monograph.resolve_concept(segment);
        },
        [&](const Abstract& context) -> const Abstract& {
          return context.resolve_concept(segment);
        });
    const Abstract& candidate = queried.resolve();
    BAIL_IF(candidate.is<Unknown>() || candidate.is<None>());
    selected = Reference<const Abstract>(candidate);

    if (separator) {
      index++;
      start = index + 1;
    }
  }
  return selected.get();
}

auto App::Language::Route::resolve(Cursor& cursor, const Abstract& context)
    const -> Option<const Abstract&> {
  auto selected = resolve_route(spelling, context);
  if (!selected) {
    auto report = cursor.create_report(anchor);
    report << "App route `"_view << spelling
           << "` does not resolve in this Package."_view;
    report.get_hint()
        << "Select one exact Package member or nested public context."_view;
    return {};
  }
  cursor.get_associations().create(anchor, *selected);
  return *selected;
}
