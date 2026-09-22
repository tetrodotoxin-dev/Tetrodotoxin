// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/interpreter/layout.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/parser/layout.hpp"
#include "tetrodotoxin/language/parser/type_reference.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Render;

auto Interpreter::Layout::parse(Cursor& cursor, Bool parameters)
    -> Option<Language::Layout&> {
  Allocator::Arena& domain = cursor.get_arena();
  Token opening = cursor.current();
  Managed::Vector<Language::Layout::Slot> slots(domain);
  auto closing = Tetrodotoxin::Language::Parser::Layout::parse(
      cursor, False,
      [&](Cursor& entry, Count, Option<Token> name_token,
          View::Vector<Tetrodotoxin::Language::Attribute> attributes) -> Bool {
        if (!attributes.is_empty() && !name_token) {
          entry.create_expression_error(
              attributes.get_data()[0].get_anchor(),
              "Pipeline Stage entry Attributes require one named entry."_view);
          return False;
        }

        auto type = Tetrodotoxin::Language::Parser::TypeReference::parse(entry);
        BAIL_IF(!type);
        View::Bytes name;
        Token focus = type->get_anchor().get_token();
        Token start = type->get_anchor().get_span().get_start();
        if (name_token) {
          name = name_token->caculate_text(entry.get_source_text());
          focus = *name_token;
          start = entry.peek(-3);
        }
        if (!attributes.is_empty()) {
          start = attributes.get_data()[0].get_anchor().get_span().get_start();
        }

        slots.insert(
            Language::Layout::Slot(
                *type, name, attributes,
                Anchor::create(
                    focus,
                    Span(start, type->get_anchor().get_span().get_end()))));
        return True;
      });
  BAIL_IF(!closing);
  return Language::Layout::create(
      domain, slots, Anchor::create(opening, Span(opening, *closing)),
      parameters);
}
