// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/index.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/interpreter/access/index.hpp"
#include "tetrodotoxin/library/interpreter/expression.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/u8.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

static Harness LibraryIndex = {
  .name = "Tetrodotoxin::Library::Language::Access::Index"_view,
};

class IndexBinding : public Library::Language::Model::Addressable {
 public:
  IndexBinding(View::Bytes name, const Library::Language::Model::Type& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto get_type() const -> const Library::Language::Model::Type& override {
    return type;
  }

 private:
  View::Bytes name;
  const Library::Language::Model::Type& type;
};

class IndexContext : public Abstract {
 public:
  explicit IndexContext(const Library::Language::Model::Addressable& binding)
      : binding(binding) {}

  auto get_name() const -> View::Bytes override { return "Index context"_view; }
  auto get_documentation() const -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes name) const -> const Abstract& override {
    if (name == binding.get_name()) {
      return binding;
    }

    return Unknown::get_unknown();
  }

 private:
  const Library::Language::Model::Addressable& binding;
};

static auto create_monograph(
    Allocator::Arena& domain,
    Library::Dialect& dialect,
    Abstract& context) -> Option<Library::Language::Monograph&> {
  Errors errors;
  Tokenizer tokenizer(domain, ""_view, "index-source.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  Anchor source_anchor = Anchor::create(Span());
  auto interpretation = dialect.interpret(
      cursor, Tetrodotoxin::Source::Documentation::get_empty(), source_anchor, context);
  if (!interpretation || !interpretation->is<Library::Language::Monograph>() ||
      !errors.is_empty()) {
    return {};
  }

  return static_cast<Library::Language::Monograph&>(*interpretation);
}

static auto parse_index(
    Allocator::Arena& domain,
    const Abstract& context,
    View::Bytes source,
    Errors& errors) -> Option<Library::Language::Access::Index&> {
  Tokenizer tokenizer(domain, source, "index.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto parsed = Library::Interpreter::Expression::parse(context, cursor);
  auto index = parsed.visit(
      []() -> Option<Library::Language::Access::Index&> { return {}; },
      [](Library::Language::Model::Pack& selected) {
        return selected.select_identity<Library::Language::Access::Index>();
      });
  if (!index || !cursor.matches(Code::Type::Terminal)) {
    return {};
  }

  return index;
}

static auto parse_pack(
    Allocator::Arena& domain,
    const Abstract& context,
    View::Bytes source,
    Errors& errors) -> Option<Library::Language::Model::Pack&> {
  Tokenizer tokenizer(domain, source, "index-value.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto parsed = Library::Interpreter::Expression::parse(context, cursor);
  BAIL_IF(!parsed || !cursor.matches(Code::Type::Terminal));
  return *parsed;
}

static auto rejects_committed_index_suffix(
    Allocator::Arena& domain,
    const Abstract& context,
    View::Bytes suffix) -> Bool {
  Errors receiver_errors;
  Tokenizer receiver_tokenizer(
      domain, "storage"_view, "index-receiver.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations receiver_associations(
      receiver_tokenizer.get_arena());
  Cursor receiver_cursor(
      receiver_tokenizer, receiver_errors, receiver_associations);
  auto receiver_pack =
      Library::Interpreter::Expression::parse(context, receiver_cursor);
  auto receiver = receiver_pack.visit(
      []() -> Option<Library::Language::Expression&> { return {}; },
      [](Library::Language::Model::Pack& selected) {
        return selected.select_identity<Library::Language::Expression>();
      });
  BAIL_IF(
      !receiver || !receiver_cursor.matches(Code::Type::Terminal) ||
      !receiver_errors.is_empty());

  Errors errors;
  Tokenizer tokenizer(domain, suffix, "invalid-index.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  Token opening = cursor.current();
  auto parsed =
      Library::Interpreter::Access::Index::parse(context, cursor, *receiver);
  Token ending = cursor.current();
  return !parsed && !errors.is_empty() &&
         ending.get_offset() > opening.get_offset();
}

PERIMORTEM_UNIT_TEST(LibraryIndex, write_only_scalar) {
  Allocator::Arena domain;
  Library::Language::Types::U8 element;
  Library::Language::Types::Access access("Access[U8]"_view, element);
  IndexBinding binding("storage"_view, access);
  IndexContext context(binding);
  Library::Dialect dialect;
  auto owner = create_monograph(domain, dialect, context);
  ASSERT(owner);
  auto& monograph = *owner;
  Errors unsigned_errors;
  auto unsigned_index =
      parse_index(domain, monograph, "storage[1]"_view, unsigned_errors);
  ASSERT(unsigned_index);
  Tokenizer unsigned_tokens(domain, "storage[1]"_view, "index.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations unsigned_associations(unsigned_tokens.get_arena());
  Cursor unsigned_cursor(
      unsigned_tokens, unsigned_errors, unsigned_associations);
  Errors source_errors;
  auto source = parse_pack(domain, monograph, "1"_view, source_errors);
  ASSERT(source);

  EXPECT(&unsigned_index->get_element_type() == &Unknown::get_unknown());
  EXPECT(&unsigned_index->resolve() == &Unknown::get_unknown());
  EXPECT(unsigned_index->is<Library::Language::Expression>());
  static_assert(__is_base_of(
      Library::Language::Model::Pack, Library::Language::Expression));
  ASSERT(
      unsigned_index->link_write(unsigned_cursor, context, element, *source));
  ASSERT(
      unsigned_index->link_write(unsigned_cursor, context, element, *source));
  EXPECT(&unsigned_index->get_element_type() == &element);
  EXPECT(&unsigned_index->get_type() == &element);
  EXPECT(&unsigned_index->resolve() == &Unknown::get_unknown());
  ASSERT_EQ(unsigned_index->get_layout().get_size(), Count(1));
  EXPECT(&*unsigned_index->get_layout().get_abstract(0) == &*unsigned_index);
  EXPECT_NOT(unsigned_index->is_complete());
  unsigned_index->finalize(unsigned_cursor);
  EXPECT(unsigned_errors.is_empty());
  EXPECT(source_errors.is_empty());

  Errors read_errors;
  auto read_index =
      parse_index(domain, monograph, "storage[1]"_view, read_errors);
  ASSERT(read_index);
  Tokenizer read_tokens(domain, "storage[1]"_view, "index.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations read_associations(read_tokens.get_arena());
  Cursor read_cursor(read_tokens, read_errors, read_associations);
  EXPECT_NOT(read_index->link(read_cursor, context));
  EXPECT_NOT(read_errors.is_empty());

  Errors signed_errors;
  auto signed_index =
      parse_index(domain, monograph, "storage[-1]"_view, signed_errors);
  ASSERT(signed_index);
  Tokenizer signed_tokens(domain, "storage[-1]"_view, "index.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations signed_associations(signed_tokens.get_arena());
  Cursor signed_cursor(signed_tokens, signed_errors, signed_associations);
  EXPECT(signed_index->link_write(signed_cursor, context, element, *source));
  EXPECT(&signed_index->get_element_type() == &element);
  EXPECT(signed_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryIndex, atomic_range_write) {
  Allocator::Arena domain;
  Library::Language::Types::U8 element;
  Library::Language::Types::Access access("Access[U8]"_view, element);
  IndexBinding binding("storage"_view, access);
  IndexContext context(binding);
  Library::Dialect dialect;
  auto owner = create_monograph(domain, dialect, context);
  ASSERT(owner);
  auto& monograph = *owner;

  Errors errors;
  auto index = parse_index(domain, monograph, "storage[1, 2]"_view, errors);
  ASSERT(index);
  ASSERT(index->get_count());
  EXPECT_NOT(index->get_range_count());
  Errors source_errors;
  auto source = parse_pack(domain, monograph, "(1, 2)"_view, source_errors);
  ASSERT(source);
  Tokenizer tokens(domain, "storage[1, 2]"_view, "index.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations associations(tokens.get_arena());
  Cursor cursor(tokens, errors, associations);

  ASSERT(index->link_write(cursor, context, element, *source));
  ASSERT(index->get_range_count());
  EXPECT_EQ(*index->get_range_count(), Count(2));
  EXPECT(&index->get_type() == &Unknown::get_unknown());
  EXPECT_NOT(index->get_write_type(element));
  EXPECT(errors.is_empty());
  EXPECT(source_errors.is_empty());

  Errors short_errors;
  auto short_source = parse_pack(domain, monograph, "1"_view, short_errors);
  ASSERT(short_source);
  Tokenizer short_tokens(domain, "storage[1, 2]"_view, "index.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations short_associations(short_tokens.get_arena());
  Cursor short_cursor(short_tokens, short_errors, short_associations);
  EXPECT_NOT(index->link_write(short_cursor, context, element, *short_source));
  EXPECT_NOT(short_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryIndex, invalid_domains) {
  Allocator::Arena domain;
  Library::Language::Types::U8 element;
  Library::Language::Types::Access access("Access[U8]"_view, element);
  Library::Language::Types::Boolean value_type;
  IndexBinding storage("storage"_view, access);
  IndexBinding value("value"_view, value_type);
  IndexContext storage_context(storage);
  IndexContext value_context(value);
  Library::Dialect dialect;

  auto receiver_owner = create_monograph(domain, dialect, value_context);
  ASSERT(receiver_owner);
  auto& receiver_source = *receiver_owner;
  Errors receiver_parse_errors;
  auto invalid_receiver = parse_index(
      domain, receiver_source, "value[1]"_view, receiver_parse_errors);
  ASSERT(invalid_receiver);
  Tokenizer receiver_tokens(domain, "value[1]"_view, "index.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations receiver_associations(receiver_tokens.get_arena());
  Cursor receiver_cursor(
      receiver_tokens, receiver_parse_errors, receiver_associations);
  EXPECT_NOT(invalid_receiver->link(receiver_cursor, value_context));
  EXPECT_NOT(receiver_parse_errors.is_empty());

  auto index_owner = create_monograph(domain, dialect, storage_context);
  ASSERT(index_owner);
  auto& index_source = *index_owner;
  Errors index_parse_errors;
  auto invalid_index = parse_index(
      domain, index_source, "storage[true]"_view, index_parse_errors);
  ASSERT(invalid_index);
  Tokenizer index_tokens(domain, "storage[true]"_view, "index.ttx"_view);
  Tetrodotoxin::Source::Lexical::Associations index_associations(index_tokens.get_arena());
  Cursor index_cursor(index_tokens, index_parse_errors, index_associations);
  EXPECT_NOT(invalid_index->link(index_cursor, storage_context));
  EXPECT_NOT(index_parse_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryIndex, committed_postfix) {
  Allocator::Arena domain;
  Library::Language::Types::U8 element;
  Library::Language::Types::Access access("Access[U8]"_view, element);
  IndexBinding storage("storage"_view, access);
  IndexContext context(storage);
  Library::Dialect dialect;
  auto owner = create_monograph(domain, dialect, context);
  ASSERT(owner);
  auto& monograph = *owner;

  EXPECT(rejects_committed_index_suffix(domain, monograph, "[]"_view));
  EXPECT(rejects_committed_index_suffix(domain, monograph, "[1"_view));
  EXPECT(rejects_committed_index_suffix(domain, monograph, "[1,]"_view));
  EXPECT(rejects_committed_index_suffix(domain, monograph, "[1, 2, 3]"_view));
}
