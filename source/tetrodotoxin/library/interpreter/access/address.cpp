// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/access/address.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Library;

auto Interpreter::Access::Address::parse(
    const Abstract&,
    Cursor& cursor,
    Language::Model::Pack& receiver) -> Option<Language::Expression&> {
  Token operation = cursor.consume();
  Token addressable = cursor.require(
      Code::Type::Addressable,
      "Address requires one addressable name after `.`."_view);
  BAIL_IF(!addressable);
  const auto& receiver_anchor = receiver.get_anchor();
  if (!receiver_anchor) {
    cursor.create_expression_error(
        Anchor::create(addressable, Span(operation, addressable)),
        "Address requires an authored receiver Anchor."_view);
    return {};
  }

  return Language::Access::Address::create_authored(
      cursor.get_arena(), receiver, addressable,
      addressable.caculate_text(cursor.get_source_text()),
      Anchor::create(
          addressable, receiver_anchor->get_span(), Span(addressable)));
}
