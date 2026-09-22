// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/return.hpp"

#include "tetrodotoxin/library/language/diagnostics.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

auto Language::Flow::Return::create_authored(
    Memory::Allocator::Arena& domain,
    Anchor anchor,
    Model::Pack& pack) -> Return& {
  return domain.construct_from<Return>(
      [&]() -> Return { return Return(anchor, pack); });
}

auto Language::Flow::Return::link(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    const Language::Model::Type& access_scope,
    const Layout& results) -> Bool {
  if (linked) {
    return True;
  }

  Model::Pack& selected = pack.get();
  BAIL_IF(!selected.link(cursor, lexical_context, access_scope));
  // Return owns produced flow, not contextual identity traversal. Reject a
  // selected Type before result fitting asks it for a value Layout.
  if (!selected.is_complete()) {
    cursor.create_expression_error(
        anchor, "Return expression did not produce value flow."_view,
        "Use a Type result only as an access receiver."_view);
    return False;
  }
  Bool fits = selected.fits(results);

  if (!fits) {
    auto report = cursor.create_report(anchor);
    report << "Return values do not fit the Function result Layout.\n"
              "Source produces: "_view;
    Language::Diagnostics::write_pack(report, selected);
    report << "\nFunction accepts: "_view;
    Language::Diagnostics::write_layout(report, results);
    report.get_hint()
        << "Return the exact ordered Types declared by the Function."_view;
    return False;
  }

  linked = True;
  return True;
}

auto Language::Flow::Return::finalize(Cursor& cursor) -> void {
  pack.get().finalize(cursor);
}
