// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/type.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

static auto resolve_receiver(const Language::Model::Pack& receiver)
    -> const Abstract& {
  const Abstract& completed = receiver.get_result();
  if (!completed.is<Unknown>() && !completed.is<None>()) {
    return completed;
  }

  auto identifier =
      receiver.select_identity<Language::Expressions::Identifier>();
  if (identifier) {
    return identifier->resolve_authored();
  }

  auto access = receiver.select_identity<Language::Access::Type>();
  return access ? access->resolve_authored() : Unknown::get_unknown();
}

static auto select_type_access(const Abstract& receiver, Core::View::Bytes name)
    -> const Abstract& {
  return receiver.resolve_concept("static"_view)
      .resolve_concept(name)
      .resolve();
}

auto Language::Access::Type::create_authored(
    Memory::Allocator::Arena& domain,
    Model::Pack& receiver,
    Token token,
    Core::View::Bytes name,
    Anchor anchor) -> Type& {
  return Expression::create_authored<Type>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Type {
        return Type(receiver, token, name, source);
      });
}

auto Language::Access::Type::link(
    Tetrodotoxin::Source::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(cursor, lexical_context, access_scope));

  const Abstract& receiver_result = receiver.get_result();
  const Abstract& result = select_type_access(receiver_result, name);
  if (result.is<Unknown>() || result.is<None>()) {
    cursor.create_expression_error(
        get_anchor(), "Type access did not select a semantic context."_view,
        "Publish the named context or Type on the receiver before linking this access."_view);
    return False;
  }

  if (selected && &selected->get() != &result) {
    cursor.create_expression_error(
        get_anchor(), "Type access cannot change its selected result."_view,
        "Keep one exact Type bound to this authored Token."_view);
    return False;
  }

  selected = Reference<const Abstract>(result);
  auto source_anchor = get_anchor();
  if (source_anchor) {
    cursor.get_associations().create(*source_anchor, result);
  }
  return True;
}

auto Language::Access::Type::get_documentation() const -> const Tetrodotoxin::Source::Documentation& {
  return selected.visit(
      []() -> const Tetrodotoxin::Source::Documentation& { return Tetrodotoxin::Source::Documentation::get_empty(); },
      [](const Reference<const Abstract>& selected) -> const Tetrodotoxin::Source::Documentation& {
        return selected.get().get_documentation();
      });
}

auto Language::Access::Type::get_type() const -> const Abstract& {
  return selected.visit(
      []() -> const Abstract& { return Unknown::get_unknown(); },
      [](const Reference<const Abstract>& selected) -> const Abstract& {
        auto pack = Language::Model::Pack::from(selected.get().resolve());
        return pack ? pack->get_type()
                    : static_cast<const Abstract&>(Unknown::get_unknown());
      });
}

auto Language::Access::Type::get_result() const -> const Abstract& {
  return selected.visit(
      []() -> const Abstract& { return Unknown::get_unknown(); },
      [](const Reference<const Abstract>& selected) -> const Abstract& {
        return selected.get();
      });
}

auto Language::Access::Type::resolve_concept(Core::View::Bytes route) const
    -> const Abstract& {
  if (route != "static"_view && route != "instance"_view) {
    return Expression::resolve_concept(route);
  }

  const Abstract& selected = resolve_authored();
  return selected.is<Unknown>() || selected.is<None>()
             ? selected
             : selected.resolve_concept(route);
}

auto Language::Access::Type::visit_concepts(
    Tetrodotoxin::Source::Abstract::Visitor visitor) const -> void {
  // These queries can complete the expression. Capture the advertised answers
  // together before entering receiver code, without copying their storage.
  const Abstract& folded = Expression::resolve_concept("folded"_view);
  const Abstract& instance = resolve_concept("instance"_view);
  const Abstract& static_scope = resolve_concept("static"_view);
  visitor("folded"_view, folded);
  visitor("instance"_view, instance);
  visitor("static"_view, static_scope);
}

auto Language::Access::Type::resolve_authored() const -> const Abstract& {
  if (selected) {
    return selected->get();
  }

  const Abstract& receiver_result = resolve_receiver(receiver);
  return receiver_result.is<Unknown>()
             ? static_cast<const Abstract&>(Unknown::get_unknown())
             : select_type_access(receiver_result, name);
}

auto Language::Access::Type::finalize(Cursor& cursor) -> void {
  receiver.finalize(cursor);
  Expression::finalize(cursor);
}
