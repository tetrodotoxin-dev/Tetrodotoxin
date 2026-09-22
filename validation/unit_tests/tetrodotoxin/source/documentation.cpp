// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/source/documentations/block.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Validation;

static Harness TtxDocumentation = {
  .name = "TTX::Documentation"_view,
};

PERIMORTEM_UNIT_TEST(TtxDocumentation, empty) {
  const Documentations::Comment& empty = Documentations::Comment::get_empty();
  const Tetrodotoxin::Source::Documentation& documentation = empty;

  EXPECT(&empty == &Documentations::Comment::get_empty());
  EXPECT(documentation.is_empty());
  EXPECT_EQ(documentation.line_count(), Count(0));
  EXPECT(documentation.get_line(0).is_empty());
}

PERIMORTEM_UNIT_TEST(TtxDocumentation, lines) {
  static constexpr Static::Vector<View::Bytes, 2> lines = {{
    "First"_view,
    "Second"_view,
  }};

  Documentations::Block documentation(lines);

  EXPECT_NOT(documentation.is_empty());
  EXPECT_EQ(documentation.line_count(), Count(2));
  EXPECT_TEXT(documentation.get_line(0), "First"_view);
  EXPECT_TEXT(documentation.get_line(1), "Second"_view);
  EXPECT(documentation.get_line(2).is_empty());
}

PERIMORTEM_UNIT_TEST(TtxDocumentation, empty_line) {
  static constexpr Static::Vector<View::Bytes, 2> lines = {{
    View::Bytes(),
    "Second"_view,
  }};

  Documentations::Block documentation(lines);

  EXPECT_NOT(documentation.is_empty());
  EXPECT(documentation.get_line(0).is_empty());
  EXPECT_TEXT(documentation.get_line(1), "Second"_view);
}

PERIMORTEM_UNIT_TEST(TtxDocumentation, comment) {
  Documentations::Comment documentation("Stable"_view);

  EXPECT_EQ(documentation.line_count(), Count(1));
  EXPECT_TEXT(documentation.get_line(0), "Stable"_view);
  EXPECT(documentation.get_line(1).is_empty());
}

PERIMORTEM_UNIT_TEST(TtxDocumentation, late_bound) {
  Static::Vector<View::Bytes, 2> lines = {{
    "Initial"_view,
    "Second"_view,
  }};

  Documentations::Block documentation(lines);
  lines[0] = "Updated"_view;

  EXPECT_TEXT(documentation.get_line(0), "Updated"_view);
  EXPECT_TEXT(documentation.get_line(1), "Second"_view);
}
