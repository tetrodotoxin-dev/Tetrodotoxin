// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/lexical/cursors/stream.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;

auto Cursors::Stream::get_interface() const -> Cursor {
  static constexpr tetrodotoxin_source_cursor_ops operations = {
    [](const void* self, U64 index) -> Token {
      return static_cast<const Cursors::Stream*>(self)->stream[index];
    },
    [](const void* self, Token token) -> perimortem_view_bytes {
      return static_cast<const Cursors::Stream*>(self)->stream.get_text(token);
    },
    [](const void* self, Span span, const Token* focus,
       tetrodotoxin_source_anchor* result) {
      *result = static_cast<const Cursors::Stream*>(self)->stream.get_anchor(
          span, focus ? Option<Token>(*focus) : Option<Token>());
    },
    [](const void* self) -> U64 {
      return static_cast<const Cursors::Stream*>(self)->errors.get_size();
    },
    [](const void* self, const tetrodotoxin_source_anchor* anchor,
       perimortem_view_bytes message, perimortem_view_bytes hint) {
      const auto& cursor = *static_cast<const Cursors::Stream*>(self);
      cursor.errors.report(
          cursor.stream.get_source_path(), cursor.stream.get_source_text(),
          anchor ? Option<Anchor>(*anchor) : Option<Anchor>(), message, hint);
    },
    [](const void* self, U64 index,
       tetrodotoxin_source_diagnostic* result) -> U8 {
      return static_cast<const Cursors::Stream*>(self)
          ->errors.get_error(index)
          .visit(
              []() -> U8 { return 0; },
              [&](Diagnostic value) -> U8 {
                *result = value;
                return 1;
              });
    }};

  return Cursor(Cursor::Api{this, &operations, 0});
}
