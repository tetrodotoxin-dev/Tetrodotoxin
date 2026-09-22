// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/dialect.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "tetrodotoxin/source/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

class DefaultDialect : public Language::Dialect {
 public:
  TTX_NAME("Default"_view);

  auto interpret(Cursor&, const Tetrodotoxin::Source::Documentation&, const Anchor&, Abstract&)
      -> Option<Language::Monograph&> override {
    return {};
  }
};

class DefaultMonograph : public Language::Monograph {
 public:
  DefaultMonograph(
      Allocator::Arena& arena,
      const Language::Dialect& dialect,
      Abstract& context)
      : Monograph(arena, dialect, Tetrodotoxin::Source::Documentation::get_empty(), context) {}

  auto get_name() const -> View::Bytes override { return "Default"_view; }
};

class SemanticContext final : public Abstract {
 public:
  TTX_CONTRACT(Context, Abstract);

  auto get_name() const -> View::Bytes override { return "Context"_view; }

  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }

  auto resolve_concept(View::Bytes route) const -> const Abstract& override {
    if (route == "provided"_view) {
      return *this;
    }
    return Unknown::get_unknown();
  }
};

class EmptyEncodingDialect final : public DefaultDialect {
 public:
  TTX_NAME("EmptyEncoding"_view);

  auto encode(const Abstract&) const -> Option<Dynamic::Bytes> override {
    return Dynamic::Bytes();
  }
};

static Harness LanguageDialect = {
  .name = "Tetrodotoxin::Language::Dialect"_view,
};

PERIMORTEM_UNIT_TEST(LanguageDialect, explicit_defaults) {
  Allocator::Arena arena;
  DefaultDialect dialect;
  SemanticContext context;
  DefaultMonograph monograph(arena, dialect, context);
  EmptyEncodingDialect empty_dialect;
  DefaultMonograph empty_monograph(arena, empty_dialect, context);
  Errors errors;
  Tokenizer tokenizer(arena, {}, "default.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  const Bool linked = monograph.link(cursor);
  const Bool finalized = monograph.finalize(cursor);
  auto unsupported = dialect.encode(monograph);
  auto missing = dialect.decode(arena, "unsupported"_view, context);
  auto empty = empty_dialect.encode(empty_monograph);
  Bool successful_empty = empty.visit(
      []() { return False; },
      [](const Dynamic::Bytes& payload) {
        return payload.is_empty() ? True : False;
      });

  EXPECT(linked);
  EXPECT(finalized);
  EXPECT(monograph.is<Tetrodotoxin::Source::Type>());
  EXPECT(monograph.get_layout().is_empty());
  EXPECT_NOT(unsupported);
  EXPECT_NOT(missing);
  EXPECT(successful_empty);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LanguageDialect, exact_layer_identity) {
  Allocator::Arena arena;
  DefaultDialect installed;
  DefaultDialect same_type;
  SemanticContext context;
  DefaultMonograph monograph(arena, installed, context);

  auto selected = monograph.get_layer(installed);
  auto rejected = monograph.get_layer(same_type);

  ASSERT(selected);
  EXPECT(&*selected == &monograph);
  EXPECT(&monograph.get_language() == &installed);
  EXPECT_NOT(rejected);
}

PERIMORTEM_UNIT_TEST(LanguageDialect, parent_context) {
  Allocator::Arena arena;
  DefaultDialect installed;
  SemanticContext context;
  DefaultMonograph monograph(arena, installed, context);

  EXPECT(installed.is<Language::Dialect>());
  EXPECT(&monograph.get_language() == &installed);
  EXPECT(&monograph.resolve_concept("provided"_view) == &context);
  EXPECT(&monograph.resolve_concept("missing"_view) == &Unknown::get_unknown());
}
