// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/algorithm/search.hpp"

#include "tests/source_protocol.h"
#include "tetrodotoxin/dialects/source/capabilities/parse.hpp"
#include "tetrodotoxin/dialects/source/dialect.hpp"
#include "tetrodotoxin/dialects/source/errors.hpp"
#include "tetrodotoxin/dialects/source/formatter.hpp"
#include "tetrodotoxin/dialects/source/input.hpp"
#include "tetrodotoxin/dialects/source/tokenizer.hpp"
#include "toolchain/validation/unit_test.hpp"
#include "ttx/concept/capabilities/borrow.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Dialects::Source;
using namespace Toolchain::Validation;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;

static Harness source_tests("Source::Capabilities");

static auto protocol(
    View::Bytes text,
    View::Vector<tetrodotoxin_source_token> tokens = {}) -> source_protocol {
  source_protocol state{};
  state.stream_representation = tetrodotoxin_source_stream_representation();
  state.token_representation = tetrodotoxin_source_token_representation();
  state.parse_representation = tetrodotoxin_source_parse_representation();
  state.diagnostics_representation =
      tetrodotoxin_source_diagnostics_representation();
  state.input = {
    {reinterpret_cast<const U8*>("foreign.ttx"), 11},
    {text.get_data(), text.get_size()}};
  state.tokens = {
    state.token_representation, reinterpret_cast<const U8*>(tokens.get_data()),
    tokens.get_size() * sizeof(tetrodotoxin_source_token), tokens.get_size()};
  state.lexicon = Stream::lexical_contract;
  return state;
}

class SourceBytes {
 public:
  View::Bytes bytes;
  auto get_data() const -> View::Bytes { return bytes; }
};

// Matching record bytes do not admit a foreign vocabulary. A diagnostic policy
// refusal must also stop before token consumption and leave every effect
// absent.
VALIDATION_TEST(source_tests, foreign_admission) {
  const Static::Vector<tetrodotoxin_source_token, 3> tokens = {
    {tetrodotoxin_source_token(0, 1, 1, 3, 4),
     tetrodotoxin_source_token(4, 1, 5, 3, 4),
     tetrodotoxin_source_token(7, 1, 8, 0, 0)}};
  auto state = protocol("one two"_view, tokens);
  auto stream = Stream(source_protocol_stream(&state));
  auto parser = Tetrodotoxin::Dialects::Source::Capabilities::Parse(
      source_protocol_parse(&state));
  auto sink = Tetrodotoxin::Dialects::Source::Capabilities::Diagnostics(
      source_protocol_diagnostics(&state));
  Count received = 0;
  auto receive = [&](Abstract) { ++received; };
  Cursor cursor(stream);
  state.lexicon.low ^= 1;
  EXPECT_EQ(parser.parse(cursor, sink, receive), Binding::Status::Rejected);
  state.lexicon = Stream::lexical_contract;
  state.refuse_diagnostics = 1;
  EXPECT_EQ(parser.parse(cursor, sink, receive), Binding::Status::Rejected);
  state.refuse_diagnostics = 0;
  EXPECT_EQ(cursor.get_abi().index, 0);
  EXPECT_EQ(received, 0);
  EXPECT_EQ(state.reports, 0);
  EXPECT_EQ(parser.parse(cursor, sink, receive), Binding::Status::Satisfied);
  EXPECT_EQ(cursor.get_abi().index, 1);
  EXPECT_EQ(received, 1);
  EXPECT_TEXT(cursor.get_text(), "two"_view);
  EXPECT(!cursor.peek(S64(-9223372036854775807LL - 1)));
  state.refuse_stream = 1;
  stream.bind<Stream>().visit(
      [&](Stream) { EXPECT(false); },
      [&](Binding::Failure failure) {
        EXPECT_EQ(failure, Binding::Failure::Rejected);
      });
}

class Outer {
 public:
  Tetrodotoxin::Dialects::Source::Capabilities::Parse inner;
  auto get_data() const -> View::Bytes { return {}; }
  auto supports(Perimortem::System::Uuid id) const -> Binding::Status {
    return id == Tetrodotoxin::Dialects::Source::Capabilities::Parse::
                       contract_id
               ? Binding::Status::Satisfied
               : Binding::Status::Unknown;
  }
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage target) const -> Binding::Status {
    if (id ==
        Tetrodotoxin::Dialects::Source::Capabilities::Parse::contract_id) {
      return Binding::provide<
          Tetrodotoxin::Dialects::Source::Capabilities::Parse>(
          Tetrodotoxin::Dialects::Source::Capabilities::Parse::provide(*this)
              .get_abi(),
          target);
    }
    return Binding::Status::Unknown;
  }
  template <typename Receiver>
  auto parse(
      Cursor& cursor,
      Tetrodotoxin::Dialects::Source::Capabilities::Diagnostics diagnostics,
      Receiver& receive) const -> Binding::Status {
    if (cursor.get_stream().get_lexicon() != Stream::lexical_contract) {
      return Binding::Status::Rejected;
    }
    auto ignore = [&](Abstract) {};
    auto status = inner.parse(cursor, diagnostics, ignore);
    if (status != Binding::Status::Satisfied) {
      return status;
    }
    auto forward = [&](Abstract projection) { receive(projection); };
    return inner.parse(cursor, diagnostics, forward);
  }
};

// The outer native provider passes its live position to two foreign parsers.
// A bounded range leaves the enclosing token untouched even after consuming
// Terminal repeatedly, and the receiver sees the final position synchronously.
VALIDATION_TEST(source_tests, nested_cursor) {
  const Static::Vector<tetrodotoxin_source_token, 4> tokens = {
    {tetrodotoxin_source_token(0, 1, 1, 3, 4),
     tetrodotoxin_source_token(4, 1, 5, 3, 4),
     tetrodotoxin_source_token(8, 1, 9, 4, 4),
     tetrodotoxin_source_token(12, 1, 13, 0, 0)}};
  auto state = protocol("one two next"_view, tokens);
  Outer outer{Tetrodotoxin::Dialects::Source::Capabilities::Parse(
      source_protocol_parse(&state))};
  auto sink = Tetrodotoxin::Dialects::Source::Capabilities::Diagnostics(
      source_protocol_diagnostics(&state));
  Cursor cursor(Stream(source_protocol_stream(&state)));
  cursor.get_abi().end = 2;
  Count received = 0;
  auto receive = [&](Abstract) {
    ++received;
    EXPECT_EQ(cursor.get_abi().index, 2);
  };
  Abstract::provide(outer)
      .bind<Tetrodotoxin::Dialects::Source::Capabilities::Parse>()
      .visit(
          [&](Tetrodotoxin::Dialects::Source::Capabilities::Parse bound) {
            EXPECT_EQ(
                bound.parse(cursor, sink, receive), Binding::Status::Satisfied);
          },
          [&](Binding::Failure) { EXPECT(false); });
  EXPECT_EQ(received, 1);
  EXPECT_EQ(cursor.get_abi().index, 2);
  EXPECT_EQ(cursor.peek(0).get_offset(), 8);
  EXPECT_EQ(cursor.consume().get_offset(), 8);
  EXPECT_EQ(cursor.consume().get_offset(), 8);
  EXPECT(!cursor.peek(1));
  EXPECT_TEXT(
      cursor.peek(-1).caculate_text(cursor.get_source_text()), "two"_view);
}

// A C parser reports through the native sink, then its source and provider die.
// Rendering must still use the copied bytes and preserve the actual consumption
// on failure. The converse call proves a C sink can receive native reports.
VALIDATION_TEST(source_tests, retained_diagnostics) {
  Errors errors;
  {
    const Static::Vector<tetrodotoxin_source_token, 2> tokens = {
      {tetrodotoxin_source_token(0, 1, 1, 1, 255),
       tetrodotoxin_source_token(1, 1, 2, 0, 0)}};
    Static::Bytes<1> text = {{'`'}};
    auto state = protocol(text, tokens);
    auto parser = Tetrodotoxin::Dialects::Source::Capabilities::Parse(
        source_protocol_parse(&state));
    Cursor cursor(Stream(source_protocol_stream(&state)));
    auto receive = [&](Abstract) { EXPECT(false); };
    EXPECT_EQ(
        parser.parse(cursor, errors.get_diagnostics(), receive),
        Binding::Status::Rejected);
    EXPECT_EQ(cursor.get_abi().index, 1);
    Static::Bytes<9> message = {{'T', 'e', 'm', 'p', 'o', 'r', 'a', 'r', 'y'}};
    errors.get_diagnostics().report(
        state.input, Anchor::create(Span()), message);
    Data::set(message.get_data(), 'x', message.get_size());
    text[0] = 'x';
  }
  EXPECT_EQ(errors.get_size(), 2);
  EXPECT_TEXT(errors.get_message(1), "Temporary"_view);
  EXPECT_TEXT(errors.get_message(0), "Expected a name."_view);
  Allocator::Arena rendering;
  const auto rendered = errors.render_message(rendering, 0);
  EXPECT(Algorithm::search(rendered, "`"_view) != Count(-1));
  EXPECT_TEXT(errors.get_source_name(0), "foreign.ttx"_view);
  auto foreign = protocol("evidence"_view);
  auto sink = Tetrodotoxin::Dialects::Source::Capabilities::Diagnostics(
      source_protocol_diagnostics(&foreign));
  sink.report(foreign.input, Anchor::create(Span()), "Native report."_view);
  EXPECT_EQ(foreign.reports, 1);
  EXPECT_TEXT(View::Bytes(foreign.message, 14), "Native report."_view);
  EXPECT_TEXT(View::Bytes(foreign.evidence, 8), "evidence"_view);
}

// The receiver acquires its result during Parse, after Source copied the input.
// Parser state, caller bytes and the Import callback all end before observation
// through that retained policy. Rebinding Stream preserves its release policy.
VALIDATION_TEST(source_tests, parse_result_lifetime) {
  Option<Policies::Borrowed> retained;
  {
    Static::Bytes<3> text = {{'o', 'n', 'e'}};
    auto state = protocol(text);
    auto parser = Tetrodotoxin::Dialects::Source::Capabilities::Parse(
        source_protocol_parse(&state));
    Errors errors;
    auto observe = [&](Abstract source) {
      source.bind<Stream>().visit(
          [&](Stream stream) {
            EXPECT_EQ(
                stream.supports<Policies::Borrowed>(),
                Binding::Status::Rejected);
            Cursor cursor(stream);
            auto receive = [&](Abstract projection) {
              projection.bind<Ttx::Concept::Capabilities::Borrow>().visit(
                  [&](Ttx::Concept::Capabilities::Borrow borrow) {
                    borrow.borrow().visit(
                        [&](Policies::Borrowed value) { retained = value; },
                        [&](Binding::Failure) { EXPECT(false); });
                  },
                  [&](Binding::Failure) { EXPECT(false); });
            };
            EXPECT_EQ(
                parser.parse(cursor, errors.get_diagnostics(), receive),
                Binding::Status::Satisfied);
          },
          [&](Binding::Failure) { EXPECT(false); });
    };
    SourceBytes bytes{
      View::Bytes(state.input.text.data, state.input.text.size)};
    const tetrodotoxin_source_input input{
      state.input.path, Abstract::provide(bytes).get_abi()};
    EXPECT_EQ(
        Dialect::importer().visit(
            &input, *tetrodotoxin_source_input_representation(), observe),
        Binding::Status::Satisfied);
    text[0] = 'x';
  }
  EXPECT(retained);
  if (retained) {
    retained->bind<Stream>().visit(
        [&](Stream stream) {
          EXPECT_TEXT(stream.get_source_text(), "one"_view);
          EXPECT_EQ(
              stream.supports<Policies::Borrowed>(),
              Binding::Status::Satisfied);
          EXPECT_EQ(stream.get_tokens().get_size(), 2);
        },
        [&](Binding::Failure) { EXPECT(false); });
    retained->release();
  }
}

// Compact limits reject before field truncation. A failed import never lends
// a partial stream whose Terminal could conceal the unsupported source tail.
VALIDATION_TEST(source_tests, compact_bounds) {
  Allocator::Arena arena;
  Static::Bytes<256> word;
  Data::set(word.get_data(), 'a', word.get_size());
  Tokenizer accepted(arena, View::Bytes(word.get_data(), 255), {});
  EXPECT(accepted.is_valid());
  EXPECT_EQ(accepted.get_tokens()[0].get_size(), 255);
  Tokenizer rejected(arena, word, {});
  EXPECT(!rejected.is_valid());
  Static::Bytes<65536> bytes;
  Data::set(bytes.get_data(), '\n', bytes.get_size());
  Tokenizer last_line(arena, View::Bytes(bytes.get_data(), 65534), {});
  EXPECT(last_line.is_valid());
  EXPECT_EQ(last_line.get_tokens()[0].get_line(), 65535);
  Tokenizer excessive_line(arena, View::Bytes(bytes.get_data(), 65535), {});
  EXPECT(!excessive_line.is_valid());
  Data::set(bytes.get_data(), ' ', bytes.get_size());
  Tokenizer last_column(arena, View::Bytes(bytes.get_data(), 65534), {});
  EXPECT(last_column.is_valid());
  EXPECT_EQ(last_column.get_tokens()[0].get_column(), 65535);
  Tokenizer excessive_column(arena, View::Bytes(bytes.get_data(), 65535), {});
  EXPECT(!excessive_column.is_valid());
  bytes[65534] = '\n';
  Tokenizer last_offset(arena, View::Bytes(bytes.get_data(), 65535), {});
  EXPECT(last_offset.is_valid());
  EXPECT_EQ(last_offset.get_tokens()[0].get_offset(), 65535);
  Tokenizer excessive_offset(arena, bytes, {});
  EXPECT(!excessive_offset.is_valid());
  auto state = protocol(word);
  auto receive = [&](Abstract) { EXPECT(false); };
  SourceBytes source_bytes{
    View::Bytes(state.input.text.data, state.input.text.size)};
  const tetrodotoxin_source_input input{
    state.input.path, Abstract::provide(source_bytes).get_abi()};
  EXPECT_EQ(
      Dialect::importer().visit(
          &input, *tetrodotoxin_source_input_representation(), receive),
      Binding::Status::Rejected);
  Tokenizer escape(arena, "\"tail\\"_view, {});
  EXPECT(escape.is_valid());
  EXPECT_EQ(escape.get_tokens()[0].get_size(), 6);
  Tokenizer attribute(arena, "@"_view, {});
  EXPECT(attribute.is_valid());
  EXPECT_EQ(attribute.get_tokens()[0].get_size(), 0);
  EXPECT_EQ(attribute.get_tokens()[1].get_offset(), 1);
}

// The public reader refuses unsupported descriptors and short buffers without
// interpreting them as the current profile. Its caller can choose another
// encoding reader later without changing Cursor's logical index contract.
VALIDATION_TEST(source_tests, described_storage) {
  const Static::Vector<tetrodotoxin_source_token, 2> tokens = {
    {tetrodotoxin_source_token(0, 1, 1, 1, 4),
     tetrodotoxin_source_token(1, 1, 2, 0, 0)}};
  auto state = protocol("a"_view, tokens);
  state.lexicon.low ^= 1;
  Formatter(Stream(source_protocol_stream(&state)))
      .format()
      .visit(
          [&](Dynamic::Bytes) { EXPECT(false); },
          [&](Binding::Failure failure) {
            EXPECT_EQ(failure, Binding::Failure::Rejected);
          });
  state.lexicon = Stream::lexical_contract;
  state.tokens.representation = state.stream_representation;
  EXPECT(!Stream(source_protocol_stream(&state)).get_tokens().is_valid());
  state.tokens.representation = state.token_representation;
  --state.tokens.size;
  EXPECT(!Stream(source_protocol_stream(&state)).get_tokens().is_valid());
  state.tokens.size = sizeof(tetrodotoxin_source_token) * 2;
  state.tokens.count = 1;
  EXPECT(!Stream(source_protocol_stream(&state)).get_tokens().is_valid());
}

// A retained byte answer can differ from the initial observation. Its release
// counter exposes both a premature release and a leaked acquisition on refusal.
class RetainedBytes {
 public:
  View::Bytes bytes;
  mutable Count reads = 0;
  mutable Count releases = 0;
  auto get_data() const -> View::Bytes {
    ++reads;
    return bytes;
  }
  auto release() const -> void { ++releases; }
};

class BorrowableBytes {
 public:
  View::Bytes bytes;
  RetainedBytes& retained;
  Binding::Status binding = Binding::Status::Satisfied;
  Binding::Status acquisition = Binding::Status::Satisfied;
  mutable Count reads = 0;
  mutable Count acquisitions = 0;
  auto get_data() const -> View::Bytes {
    ++reads;
    return bytes;
  }
  auto supports(Perimortem::System::Uuid id) const -> Binding::Status {
    return id == Ttx::Concept::Capabilities::Borrow::contract_id
               ? binding
               : Binding::Status::Unknown;
  }
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage target) const -> Binding::Status {
    if (id != Ttx::Concept::Capabilities::Borrow::contract_id) {
      return Binding::Status::Unknown;
    }
    if (binding != Binding::Status::Satisfied) {
      return binding;
    }
    return Binding::provide<Ttx::Concept::Capabilities::Borrow>(
        Ttx::Concept::Capabilities::Borrow::provide(*this).get_abi(), target);
  }
  auto borrow() const
      -> Perimortem::Utility::Result<Policies::Borrowed, Binding::Failure> {
    ++acquisitions;
    if (acquisition != Binding::Status::Satisfied) {
      return static_cast<Binding::Failure>(acquisition);
    }
    return Policies::Borrowed::provide(retained);
  }
};

VALIDATION_TEST(source_tests, borrow_input) {
  RetainedBytes bytes{"retained"_view};
  BorrowableBytes owner{"initial"_view, bytes};
  const tetrodotoxin_source_input input{{}, Abstract::provide(owner).get_abi()};
  Option<Policies::Borrowed> stream_owner;
  auto receive = [&](Abstract graph) {
    graph.bind<Stream>().visit(
        [&](Stream stream) {
          EXPECT_EQ(
              stream.get_source_text().get_data(), bytes.bytes.get_data());
          EXPECT_TEXT(stream.get_source_text(), "retained"_view);
          EXPECT_EQ(stream.get_tokens()[0].get_size(), 8);
        },
        [&](Binding::Failure) { EXPECT(false); });
    graph.bind<Ttx::Concept::Capabilities::Borrow>().visit(
        [&](Ttx::Concept::Capabilities::Borrow borrow) {
          borrow.borrow().visit(
              [&](Policies::Borrowed value) { stream_owner = value; },
              [&](Binding::Failure) { EXPECT(false); });
        },
        [&](Binding::Failure) { EXPECT(false); });
  };
  EXPECT_EQ(
      Dialect::importer().visit(
          &input, *tetrodotoxin_source_input_representation(), receive),
      Binding::Status::Satisfied);
  EXPECT_EQ(owner.reads, 0);
  EXPECT_EQ(owner.acquisitions, 1);
  EXPECT_EQ(bytes.reads, 1);
  EXPECT_EQ(bytes.releases, 0);
  EXPECT(stream_owner);
  if (stream_owner) {
    stream_owner->release();
  }
  EXPECT_EQ(bytes.releases, 1);

  Static::Bytes<256> excessive;
  Data::set(excessive.get_data(), 'a', excessive.get_size());
  RetainedBytes unsupported{excessive};
  BorrowableBytes refused{"initial"_view, unsupported};
  const tetrodotoxin_source_input invalid{
    {}, Abstract::provide(refused).get_abi()};
  auto never = [&](Abstract) { EXPECT(false); };
  EXPECT_EQ(
      Dialect::importer().visit(
          &invalid, *tetrodotoxin_source_input_representation(), never),
      Binding::Status::Rejected);
  EXPECT_EQ(unsupported.releases, 1);
}

// Unknown and Rejected from either binding or acquisition both choose a copy.
// The imported Stream must then survive mutation of the original byte buffer.
VALIDATION_TEST(source_tests, borrow_copy_fallback) {
  for (Count step = 0; step < 2; ++step) {
    for (Count status = 0; status < 2; ++status) {
      Static::Bytes<4> text = {{'c', 'o', 'p', 'y'}};
      RetainedBytes unavailable{"unused"_view};
      BorrowableBytes owner{text, unavailable};
      const auto failure =
          status ? Binding::Status::Rejected : Binding::Status::Unknown;
      if (step) {
        owner.acquisition = failure;
      } else {
        owner.binding = failure;
      }
      const tetrodotoxin_source_input input{
        {}, Abstract::provide(owner).get_abi()};
      auto receive = [&](Abstract graph) {
        Data::set(text.get_data(), 'x', text.get_size());
        graph.bind<Stream>().visit(
            [&](Stream stream) {
              EXPECT_TEXT(stream.get_source_text(), "copy"_view);
              EXPECT(stream.get_source_text().get_data() != text.get_data());
            },
            [&](Binding::Failure) { EXPECT(false); });
      };
      EXPECT_EQ(
          Dialect::importer().visit(
              &input, *tetrodotoxin_source_input_representation(), receive),
          Binding::Status::Satisfied);
      EXPECT_EQ(owner.reads, 1);
      EXPECT_EQ(owner.acquisitions, step);
      EXPECT_EQ(unavailable.reads, 0);
      EXPECT_EQ(unavailable.releases, 0);
    }
  }
}
