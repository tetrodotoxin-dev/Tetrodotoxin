// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/enumeration.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/dialect.hpp"
#include "tetrodotoxin/library/builtin/enum/name.hpp"
#include "tetrodotoxin/library/builtin/enum/size.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"
#include "tetrodotoxin/source/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  auto interpreted = workspace.interpret_source(
      errors, "EnumerationTest"_view, "enumeration.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto parse_authored(
    Allocator::Arena& lexical,
    Dialect& dialect,
    Workspace& context,
    Errors& errors,
    View::Bytes source) -> Option<Language::Monograph&> {
  Tokenizer tokenizer(lexical, source, "enumeration.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  if (!cursor.get_code().is_comment()) {
    return {};
  }

  Token source_opening = cursor.current();
  const Tetrodotoxin::Source::Documentation& documentation =
      Tetrodotoxin::Language::Parser::Comment::parse(cursor);
  Token dialect_declaration = cursor.current();
  if (Tetrodotoxin::Language::Parser::Dialect::parse(cursor) !=
      "Library"_view) {
    return {};
  }

  Anchor source_anchor = Anchor::create(
      dialect_declaration, Span(source_opening, cursor.peek(-1)));
  auto interpretation =
      dialect.interpret(cursor, documentation, source_anchor, context);
  if (!interpretation || !cursor.matches(Code::Type::Terminal) ||
      !interpretation->is<Language::Monograph>() || !errors.is_empty()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpretation);
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return !monograph && !errors.is_empty() &&
         retains_library_source(workspace, "EnumerationTest"_view);
}

static auto rejects_link(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto& dialect = get_library_dialect(*workspace_toolchain);
  Allocator::Arena lexical;
  auto monograph = parse_authored(lexical, dialect, workspace, errors, source);
  if (!monograph) {
    return False;
  }

  Allocator::Arena completion;
  Tokenizer tokenizer(completion, source, "enumeration.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  Bool linked = monograph->link(cursor);
  return !linked && !errors.is_empty() &&
         &workspace.resolve_concept("EnumerationTest"_view) ==
             &Unknown::get_unknown();
}

static auto rejects_completion_without_cases(View::Bytes source) -> Bool {
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto& dialect = get_library_dialect(*workspace_toolchain);
  Allocator::Arena lexical;
  auto owner = parse_authored(lexical, dialect, workspace, errors, source);
  if (!owner) {
    return False;
  }

  auto& monograph = *owner;
  Allocator::Arena completion;
  Tokenizer tokenizer(completion, source, "enumeration.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  Bool linked = monograph.link(cursor);
  const Abstract& selected = monograph.resolve_concept("Bad"_view);
  auto enumeration = selected.select<Language::Types::Enumeration>();
  if (!enumeration) {
    return False;
  }

  Bool finalized = linked ? monograph.finalize(cursor) : False;
  return !finalized && enumeration->get_cases().is_empty() &&
         !errors.is_empty() &&
         &workspace.resolve_concept("EnumerationTest"_view) ==
             &Unknown::get_unknown();
}

static Harness EnumerationTests = {
  .name = "Tetrodotoxin::Library::Language::Types::Enumeration"_view,
};

PERIMORTEM_UNIT_TEST(EnumerationTests, signed_aliases) {
  static constexpr View::Bytes source =
      "// Enumeration test.\n"
      "dialect : Library;\n"
      "public Offset : enum[S8] {\n"
      "  low = -128;\n"
      "  zero = 0;\n"
      "  high = 127;\n"
      "  hexadecimal = 0x7F;\n"
      "  same = 127;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const Abstract& selected = monograph->resolve_concept("Offset"_view);
  ASSERT(selected.is<Language::Types::Enumeration>());
  const auto& offset =
      static_cast<const Language::Types::Enumeration&>(selected);
  auto cases = offset.get_cases();
  ASSERT_EQ(cases.get_size(), Count(5));
  Static::Vector<S64, 5> expected = {{-128, 0, 127, 127, 127}};
  for (Count i = 0; i < cases.get_size(); i++) {
    const Abstract& resolved = cases.get_data()[i].get().resolve();
    ASSERT(resolved.is<Language::Constants::Enumeration>());
    const auto& constant =
        static_cast<const Language::Constants::Enumeration&>(resolved);
    EXPECT(&constant.get_type() == &offset);
    EXPECT_EQ(S64(constant.get_value()), expected[i]);
  }
  EXPECT(&cases.get_data()[2].get() != &cases.get_data()[4].get());
  EXPECT(
      &cases.get_data()[2].get().resolve() !=
      &cases.get_data()[4].get().resolve());

  Count generated = 0;
  Option<const Language::Model::Callable&> name_callable;
  for (const Reference<Abstract>& binding :
       offset.get_callables(Tetrodotoxin::Language::Visibility::Public)) {
    auto callable = binding.get().select<Language::Model::Callable>();
    ASSERT(callable);
    if (callable->get_name() == "get_name"_view) {
      EXPECT(callable->declares_self());
      EXPECT(callable->is<Builtin::Enum::Name>());
      name_callable = *callable;
      generated++;
    }
  }
  EXPECT_EQ(generated, Count(1));

  const Abstract& size =
      offset.resolve_concept("static"_view).resolve_concept("size"_view);
  ASSERT(size.is<Builtin::Enum::Size>());
  auto size_addressable = size.select<Language::Model::Addressable>();
  ASSERT(size_addressable);
  auto size_constant = size_addressable->get_constant();
  ASSERT(
      size_constant &&
      size_constant->is_identity<Language::Constants::Unsigned>());
  EXPECT_EQ(
      static_cast<const Language::Constants::Unsigned&>(*size_constant)
          .get_value(),
      U64(5));

  ASSERT(name_callable);
  Allocator::Arena folded_domain;
  Language::Constants::Enumeration& duplicate =
      Language::Constants::Enumeration::create_synthetic(
          folded_domain, offset, U64(127));
  Language::Model::Pack& empty =
      Language::Model::Pack::create_empty(folded_domain);
  auto folded = name_callable->fold_call(folded_domain, duplicate, empty);
  ASSERT(folded && folded->is_identity<Language::Constants::Bytes>());
  EXPECT_TEXT(
      static_cast<const Language::Constants::Bytes&>(*folded).get_value(),
      "high"_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnumerationTests, binary_boundaries) {
  static constexpr View::Bytes source =
      "// Enumeration test.\n"
      "dialect : Library;\n"
      "public UnsignedEdge : enum[U64] {\n"
      "  decimal = 18446744073709551615;\n"
      "  hexadecimal = 0xFFFFFFFFFFFFFFFF;\n"
      "}\n"
      "public SignedEdge : enum[S64] {\n"
      "  low = -9223372036854775808;\n"
      "  high = 9223372036854775807;\n"
      "}"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const Abstract& unsigned_identity =
      monograph->resolve_concept("UnsignedEdge"_view);
  const Abstract& signed_identity =
      monograph->resolve_concept("SignedEdge"_view);
  ASSERT(unsigned_identity.is<Language::Types::Enumeration>());
  ASSERT(signed_identity.is<Language::Types::Enumeration>());
  const auto& unsigned_edge =
      static_cast<const Language::Types::Enumeration&>(unsigned_identity);
  const auto& signed_edge =
      static_cast<const Language::Types::Enumeration&>(signed_identity);
  auto unsigned_cases = unsigned_edge.get_cases();
  auto signed_cases = signed_edge.get_cases();
  ASSERT_EQ(unsigned_cases.get_size(), Count(2));
  ASSERT_EQ(signed_cases.get_size(), Count(2));
  for (Count i = 0; i < unsigned_cases.get_size(); i++) {
    const auto& constant = static_cast<const Language::Constants::Enumeration&>(
        unsigned_cases.get_data()[i].get().resolve());
    EXPECT(&constant.get_type() == &unsigned_edge);
    EXPECT_EQ(constant.get_value(), U64(-1));
  }
  const auto& low = static_cast<const Language::Constants::Enumeration&>(
      signed_cases.get_data()[0].get().resolve());
  const auto& high = static_cast<const Language::Constants::Enumeration&>(
      signed_cases.get_data()[1].get().resolve());
  EXPECT(&low.get_type() == &signed_edge);
  EXPECT(&high.get_type() == &signed_edge);
  EXPECT_EQ(S64(low.get_value()), S64(-9223372036854775807LL - 1));
  EXPECT_EQ(S64(high.get_value()), S64(9223372036854775807LL));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnumerationTests, overflow_rejection) {
  static constexpr Static::Vector<View::Bytes, 7> sources = {{
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[U8] { value = 256; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[S8] { value = 128; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[S8] { value = -129; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[U8] { value = -1; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[U64] { value = 18446744073709551616; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[S64] { value = 9223372036854775808; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[S64] { value = 0x8000000000000000; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_completion_without_cases(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(EnumerationTests, invalid_storage) {
  static constexpr Static::Vector<View::Bytes, 5> sources = {{
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[Bool] { value = 0; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[R32] { value = 0; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Packet : struct {}\n"
    "public Bad : enum[Packet] { value = 0; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Mode : enum[U8] { value = 0; }\n"
    "public Bad : enum[Mode] { value = 0; }"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Bad : enum[Fixed] { value = 0; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(EnumerationTests, declaration_order) {
  static constexpr View::Bytes source =
      "// Enumeration test.\n"
      "dialect : Library;\n"
      "private Hidden : enum[S16] { hidden = -1; }\n"
      "public First : enum[U16] { first = 1; }\n"
      "public Second : enum[U32] { second = 2; }"_view;
  Tetrodotoxin::Library::Dialect workspace_toolchain_library;
  auto workspace_toolchain =
      Validation::create_library_toolchain(workspace_toolchain_library);
  Workspace workspace(*workspace_toolchain);
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  const auto& source_type = monograph->get_source();
  const Abstract& first = monograph->resolve_concept("First"_view);
  const Abstract& second = monograph->resolve_concept("Second"_view);
  ASSERT(first.is<Language::Types::Enumeration>());
  ASSERT(second.is<Language::Types::Enumeration>());
  auto types = source_type.get_types();
  ASSERT(types != types.end());
  EXPECT_TEXT((*types).get().get_name(), "Hidden"_view);
  ++types;
  ASSERT(types != types.end());
  EXPECT(&(*types).get() == &first);
  ++types;
  ASSERT(types != types.end());
  EXPECT(&(*types).get() == &second);
  EXPECT(&monograph->resolve_concept("Hidden"_view) == &Unknown::get_unknown());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnumerationTests, root_collisions) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Enumeration test.\ndialect : Library;\n"
    "public Mode : enum[U8] {}\n"
    "private Mode : enum[S8] {}"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Packet : enum[U8] {}\n"
    "public Packet : struct {}"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public Packet : struct {}\n"
    "public Packet : enum[U8] {}"_view,
    "// Enumeration test.\ndialect : Library;\n"
    "public U8 : enum[U8] {}"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(EnumerationTests, duplicate_names) {
  static constexpr View::Bytes source =
      "// Enumeration test.\n"
      "dialect : Library;\n"
      "public Mode : enum[U8] { ready = 1; ready = 2; }"_view;
  EXPECT(rejects_interpretation(source));
}

PERIMORTEM_UNIT_TEST(EnumerationTests, malformed_grammar) {
  static constexpr Static::Vector<View::Bytes, 12> sources = {{
    "// Enumeration test.\ndialect : Library; public Mode enum[U8] {}"_view,
    "// Enumeration test.\ndialect : Library; public Mode : wrong[U8] {}"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum U8] {}"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[] {}"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[U8 {}"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[U8] ready = 1; }"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[U8] { public ready = 1; }"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[U8] { Ready = 1; }"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[U8] { ready 1; }"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[U8] { ready = true; }"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[U8] { ready = 1 + 2; }"_view,
    "// Enumeration test.\ndialect : Library; public Mode : enum[U8] { ready = 1;"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}
