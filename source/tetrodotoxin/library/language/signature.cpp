// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/signature.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

auto Language::Signature::create_authored(
    Allocator::Arena& domain,
    const Abstract& host,
    Model::Layout& parameters,
    Model::Layout& results) -> Signature& {
  return create(domain, host, parameters, results);
}

auto Language::Signature::create(
    Allocator::Arena& domain,
    const Abstract& host,
    Model::Layout& parameters,
    Model::Layout& results) -> Signature& {
  return domain.construct_from<Signature>(
      [&]() -> Signature { return Signature(host, parameters, results); });
}

auto Language::Signature::link_restored() -> Bool {
  BAIL_IF(!parameters.link_restored(host, True));
  auto first = parameters.get_abstract(0);
  auto self = first ? first->select<Tetrodotoxin::Source::Addressable>()
                    : Option<const Tetrodotoxin::Source::Addressable&>();
  return results.link_restored(host, False, self);
}

auto Language::Signature::link(Cursor& cursor) -> Bool {
  // Both models run so one malformed parameter cannot hide an independent
  // result diagnostic. Each model owns idempotence for its exact staged edges.
  Bool parameters_linked = parameters.link_parameters(cursor, host);
  auto first = parameters_linked ? parameters.get_abstract(0)
                                 : Option<const Abstract&>();
  auto self = first ? first->select<Tetrodotoxin::Source::Addressable>()
                    : Option<const Tetrodotoxin::Source::Addressable&>();
  Bool results_linked = results.link_types(cursor, host, self);
  return parameters_linked && results_linked;
}

auto Language::Signature::validate_publication(Cursor& cursor) const -> Bool {
  Bool parameters_valid = parameters.validate_publication(cursor, host);
  Bool results_valid = results.validate_publication(cursor, host);
  return parameters_valid && results_valid;
}

auto Language::Signature::declares_self() const -> Bool {
  return parameters.declares_self();
}

auto Language::Signature::is_linked() const -> Bool {
  return parameters.is_linked() && results.is_linked();
}
