// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/layout.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/parser/layout.hpp"
#include "tetrodotoxin/library/interpreter/type_reference.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Layout::parse_model(
    Cursor& cursor,
    const Tetrodotoxin::Source::Abstract& host,
    Bool parameters) -> Core::Option<Language::Model::Layout&> {
  Memory::Allocator::Arena& domain = cursor.get_arena();
  Token opening = cursor.current();
  Memory::Managed::Vector<Language::Model::Layout::Slot> slots(domain);
  auto closing = Tetrodotoxin::Language::Parser::Layout::parse(
      cursor, True,
      [&](Cursor& entry, Count index, Core::Option<Token> name_token,
          Core::View::Vector<Tetrodotoxin::Language::Attribute> attributes)
          -> Bool {
        if (entry.matches(Code::Type::Self)) {
          Bool bracketed =
              name_token && name_token->get_code() == Code::Type::Self;
          Bool scalar_result = !parameters && !name_token;
          if (index != 0 || (!bracketed && !scalar_result)) {
            entry.create_token_error(
                "Library `self` must be the first Function Layout entry."_view);
            return False;
          }

          Token self = entry.consume();
          slots.insert(
              Language::Model::Layout::Slot(
                  {}, Anchor::create(Span(self)), "self"_view, attributes));
          return True;
        }

        if (!entry.matches(Code::Type::Type)) {
          entry.create_token_error(
              "Library Layout entries require one Type reference."_view);
          return False;
        }

        Token slot_opening = name_token ? entry.peek(-3) : entry.current();
        auto type = TypeReference::parse(host, entry);
        BAIL_IF(!type);

        Core::View::Bytes name;
        Anchor slot_anchor = type->get_anchor();
        if (name_token) {
          name = name_token->caculate_text(entry.get_source_text());
          slot_anchor = Anchor::create(
              *name_token,
              Span(slot_opening, type->get_anchor().get_span().get_end()));
        }

        slots.insert(
            Language::Model::Layout::Slot(
                *type, slot_anchor, name, attributes));
        return True;
      });
  BAIL_IF(!closing);

  if (parameters && !slots.is_empty() && slots.at(0).get_name().is_empty()) {
    cursor.create_expression_error(
        slots.at(0).get_type_anchor(),
        "Library Function parameters require one Named Layout."_view,
        "Use `[]` for no parameters or name every entry as `.name : Type`."_view);
    return {};
  }
  if (!parameters && !slots.is_empty() && !slots.at(0).has_type_reference() &&
      slots.get_size() != 1) {
    cursor.create_expression_error(
        slots.at(0).get_type_anchor(),
        "Library `[self]` must be the complete Function result Layout."_view,
        "Return only the receiver reference or use authored result Types."_view);
    return {};
  }

  return Language::Model::Layout::create_authored(
      domain, slots, Anchor::create(opening, Span(opening, *closing)),
      parameters);
}
