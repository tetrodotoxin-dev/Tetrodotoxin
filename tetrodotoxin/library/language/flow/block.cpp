// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/block.hpp"

#include "tetrodotoxin/library/language/flow/range_loop.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Language::Flow::Block::create_authored(
    Allocator::Arena& domain,
    const Abstract& lexical_context,
    Language::Model::Callable& function,
    const Language::Model::Type& access_scope,
    Option<Reference<const Abstract>> enclosing_loop) -> Block& {
  return domain.construct_from<Block>([&]() -> Block {
    return Block(
        domain, lexical_context, function, access_scope, enclosing_loop);
  });
}

auto Language::Flow::Block::retain_authored_statement(Statement statement)
    -> void {
  statements.insert(statement);
}

auto Language::Flow::Block::complete_authored(Anchor selected) -> void {
  anchor = selected;
}

auto Language::Flow::Block::link(Cursor& cursor) -> Bool {
  if (linked) {
    return True;
  }

  Bool failed = False;
  Bool unreachable_reported = False;
  linked_prefix_size = 0;
  auto ordered = statements.get_view();
  // One ordered pass establishes lexical visibility and reachability together.
  // The prefix advances before each owner links, so each Statement can share
  // the Block context without recovering a concrete owner category.
  for (Count index = 0; index < ordered.get_size(); index++) {
    linked_prefix_size = index;
    Statement& statement = statements.at(index);
    auto name = statement.get_binding_name();

    // Catch shadowed names and report them back to the user as an error.
    // Checking for shadowing is straight forward as resolve_concept will expose
    // any name duplicates with the benefit of giving us the shadowed object for
    // logging help info.
    const Abstract& shadowed =
        name ? resolve_concept(*name) : Unknown::get_unknown();
    if (!shadowed.is<Unknown>() && !shadowed.is<None>()) {
      auto report = cursor.create_report(statement.get_anchor());
      report << "Library Local name shadows a reachable lexical binding."_view;
      auto& note = report.get_hint();
      note << "Rename this Local so every enclosing name remains "
              "unambiguous."_view;

      // Check if we have an associated textual original so we can log location
      // information to aid in debugging.
      auto original = cursor.get_associations().find(shadowed);
      if (original) {
        Token focus = original->get_token();
        if (!focus) {
          focus = original->get_span().get_start();
        }
        if (focus) {
          note << " Original declaration: "_view << cursor.get_source_path()
               << ":"_view << focus.get_line() << ":"_view << focus.get_column()
               << "."_view;
        }
      }
      failed = True;
    }
    failed |= !statement.link(cursor, *this);

    if (!unreachable_reported && !statement.reaches_next() &&
        index + 1 < ordered.get_size()) {
      cursor.create_expression_error(
          ordered.get_data()[index + 1].get_anchor(),
          "Library Statement cannot be reached from the preceding flow."_view,
          "Remove it or make the preceding control flow continue."_view);
      unreachable_reported = True;
      failed = True;
    }
  }
  linked_prefix_size = ordered.get_size();

  linked = !failed;
  return linked;
}

auto Language::Flow::Block::finalize(Cursor& cursor) -> void {
  // Statement captured the exact owner operation when grammar selected it, so
  // finalization preserves source order without rediscovering concrete kinds.
  for (Count index = 0; index < statements.get_size(); index++) {
    statements.at(index).finalize(cursor);
  }
}

auto Language::Flow::Block::reaches_next_statement() const -> Bool {
  auto ordered = statements.get_view();
  return ordered.is_empty() ||
         ordered.get_data()[ordered.get_size() - 1].reaches_next();
}

auto Language::Flow::Block::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  // The active prefix follows source order. A Local becomes queryable
  // only after every preceding Statement links. Keeping this phase fact on the
  // real Block avoids a wrapper context for every Statement membership.
  auto ordered = statements.get_view();
  Count visible = linked_prefix_size < ordered.get_size() ? linked_prefix_size
                                                          : ordered.get_size();
  for (Count index = visible; index > 0; index--) {
    const Statement& statement = ordered.get_data()[index - 1];
    auto name = statement.get_binding_name();
    if (!name || *name != route) {
      continue;
    }

    auto binding = statement.get_binding();
    if (!binding) {
      return Unknown::get_unknown();
    }

    return *binding;
  }

  return lexical_context.resolve_concept(route);
}

auto Language::Flow::Block::resolve_authored_context(
    View::Bytes route,
    Count offset) const -> const Abstract& {
  auto ordered = statements.get_view();
  for (Count index = ordered.get_size(); index > 0; index--) {
    const Statement& statement = ordered.get_data()[index - 1];
    Span span = statement.get_anchor().get_span();
    if (span && span.get_offset() >= offset) {
      continue;
    }

    auto name = statement.get_binding_name();
    if (!name || *name != route) {
      continue;
    }

    auto binding = statement.get_binding();
    return binding ? *binding : statement.get_root();
  }

  auto parent = lexical_context.select<Block>();
  if (parent) {
    return parent->resolve_authored_context(route, offset);
  }

  auto loop = lexical_context.select<RangeLoop>();
  return loop ? loop->resolve_authored_context(route, offset)
              : lexical_context.resolve_concept(route);
}
