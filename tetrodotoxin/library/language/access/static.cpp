// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/static.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

auto Language::Access::Static::can_bind(const Abstract& binding) const -> Bool {
  Core::View::Bytes name = binding.get_name();
  BAIL_IF(name.is_empty());

  auto selected = bindings.find(name);
  return !selected || &selected->value.semantic.get() == &binding;
}

auto Language::Access::Static::bind(Abstract& binding, Bool published) -> Bool {
  BAIL_IF(!can_bind(binding));

  auto selected = bindings.find(binding.get_name());
  if (selected) {
    selected->value.published |= published;
    return True;
  }

  return bindings.insert(binding.get_name(), Binding(binding, published)) !=
         nullptr;
}

auto Language::Access::Static::is_published(const Abstract& binding) const
    -> Bool {
  auto selected = bindings.find(binding.get_name());
  return selected && &selected->value.semantic.get() == &binding &&
         selected->value.published;
}

auto Language::Access::Static::resolve_published(Core::View::Bytes name) const
    -> const Abstract& {
  auto selected = bindings.find(name);
  if (selected && selected->value.published) {
    return selected->value.semantic.get();
  }
  return selected || completed ? static_cast<const Abstract&>(None::get_none())
                               : Unknown::get_unknown();
}

auto Language::Access::Static::resolve_concept(Core::View::Bytes name) const
    -> const Abstract& {
  auto selected = bindings.find(name);
  if (selected) {
    return selected->value.semantic.get();
  }
  return completed ? static_cast<const Abstract&>(None::get_none())
                   : Unknown::get_unknown();
}

auto Language::Access::Static::visit_concepts(
    Tetrodotoxin::Source::Abstract::Visitor visitor) const -> void {
  for (Count entry = 0; entry < bindings.get_size(); entry++) {
    const auto* selected = bindings.get_entry(entry);
    if (selected != nullptr && selected->value.published) {
      const Abstract& semantic = selected->value.semantic.get();
      visitor(semantic.get_name(), semantic);
    }
  }
}
