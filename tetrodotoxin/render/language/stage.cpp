// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/stage.hpp"

#include "tetrodotoxin/render/language/attributes.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Render;

auto Language::Stage::create(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Layout& parameters,
    Layout& results) -> Stage& {
  return domain.construct_from<Stage>(
      [&]() { return Stage(definition, parameters, results); });
}

auto Language::Stage::link(Cursor& cursor) -> Bool {
  if (linked) {
    return True;
  }

  Bool valid = Attributes::validate(
      cursor, definition.get_attributes(), Attributes::Placement::Stage);
  valid &= parameters.link(cursor, definition.get_host());
  valid &= results.link(cursor, definition.get_host());
  linked = valid;
  return valid;
}

auto Language::Stage::link_restored() -> Bool {
  if (linked) {
    return True;
  }

  BAIL_IF(
      !Attributes::accepts(
          definition.get_attributes(), Attributes::Placement::Stage) ||
      !parameters.link_restored(definition.get_host()) ||
      !results.link_restored(definition.get_host()));
  linked = True;
  return True;
}

auto Language::Stage::resolve() const -> const Abstract& {
  return linked ? static_cast<const Abstract&>(*this)
                : static_cast<const Abstract&>(Unknown::get_unknown());
}

auto Language::Stage::resolve_concept(View::Bytes name) const
    -> const Abstract& {
  const Abstract& parameter = parameters.resolve_named(name);
  return parameter.is<Unknown>() || parameter.is<None>()
             ? definition.get_host().resolve_concept(name)
             : parameter;
}
