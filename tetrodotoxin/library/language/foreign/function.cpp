// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/foreign.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

using Tetrodotoxin::Language::Visibility;

auto Language::Foreign::Function::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Signature& signature,
    View::Bytes abi) -> Function& {
  return create(domain, definition, signature, abi);
}

auto Language::Foreign::Function::create(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Signature& signature,
    View::Bytes abi) -> Function& {
  return domain.construct_from<Function>(
      [&]() -> Function { return Function(definition, signature, abi); });
}

auto Language::Foreign::Function::link(Cursor& cursor) -> Bool {
  if (linked) {
    return True;
  }
  BAIL_IF(!signature.link(cursor));
  linked = True;
  return True;
}

auto Language::Foreign::Function::link_restored_declaration_signature()
    -> Bool {
  BAIL_IF(!signature.link_restored());
  linked = True;
  return True;
}

auto Language::Foreign::Function::resolve() const -> const Abstract& {
  return linked ? static_cast<const Abstract&>(*this) : Unknown::get_unknown();
}

auto Language::Foreign::Function::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  return Abstract::resolve_concept(route);
}
