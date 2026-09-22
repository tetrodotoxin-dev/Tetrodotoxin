// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/namespace.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library::Language;

auto Types::Namespace::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition) -> Namespace& {
  return domain.construct_from<Namespace>(
      [&]() -> Namespace { return Namespace(domain, definition); });
}

auto Types::Namespace::create_restored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition) -> Namespace& {
  return create_authored(domain, definition);
}

auto Types::Namespace::retain_binding(
    Abstract& binding,
    Tetrodotoxin::Language::Definition& definition,
    Category category,
    Cursor& cursor) -> Bool {
  if (category != Category::Type) {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Library Namespace bodies contain only Types and Aliases."_view,
        "Move state and Callables into an ordinary Structure or Object."_view);
    return False;
  }
  return Composite::retain_binding(binding, definition, category, cursor);
}

auto Types::Namespace::complete_body() -> void {
  complete_field_layout();
}

auto Types::Namespace::create_default(Allocator::Arena&) const
    -> Option<Model::Pack&> {
  return {};
}
