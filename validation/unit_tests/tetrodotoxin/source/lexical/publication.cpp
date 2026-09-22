// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/tetrodotoxin/source/lexical/publication.h"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/source/contents/memory.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/lexical/tokenization.hpp"
#include "ttx/semantic/flows/copy.hpp"
#include "ttx/semantic/transport/shared.hpp"
#include "validation/unit_tests/tetrodotoxin/source/content.h"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Data::Form;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Semantic::Ownership;
using namespace Ttx::Semantic::Transport;

static Validation::Harness LexicalPublication = {
  .name = "Source::Lexical::Publication"_view};
static constexpr auto bytes = Schema::range(Native<U8>::reference, 8, 1, 8, 1);
static constexpr auto empty = Schema::range(Native<U8>::reference, 0, 1, 0, 1);
static constexpr auto& form = Compiled<bytes>::get_representation();
static constexpr auto& empty_form = Compiled<empty>::get_representation();
static const Source::Lexical::Tokenization provider;

static auto tokenize(Abstract source)
    -> Utility::Result<Publication, Binding::Failure> {
  return Abstract::provide(provider).bind<Source::Tokenization>().visit(
      [&](Source::Tokenization service) { return service.tokenize(source); },
      [](Binding::Failure failure)
          -> Utility::Result<Publication, Binding::Failure> {
        return failure;
      });
}

PERIMORTEM_UNIT_TEST(LexicalPublication, borrowed_input) {
  const auto text = "public f"_view;
  const Source::Contents::Memory input(text, form);
  const auto source = Abstract::provide(input);
  tokenize(source).visit(
      [&](Publication& output) {
        // C negotiates the entire Cursor API, then receives copied Tokens
        // through its thunks. The Cursor keeps the provider's array private.
        tetrodotoxin_source_cursor api = {};
        ASSERT(
            source_read_cursor(output.get_query(), &api) ==
            TTX_BINDING_SATISFIED);
        const Source::Lexical::Cursor cursor(api);
        const auto first = cursor.get_token();
        EXPECT(cursor.get_anchor(first).get_source() == source);
        EXPECT(first.get_code() == Source::Lexical::Code::Type::Public);
        EXPECT_EQ(
            cursor.get_anchor(cursor.get_token(1)).get_extent().get_offset(),
            U64(7));
        EXPECT_EQ(
            cursor.get_anchor(cursor.get_token(2)).get_extent().get_offset(),
            U64(8));

        // C advances its own record. The C++ cursor still observes its fork's
        // original index, even though both share the same token provider.
        EXPECT_EQ(source_consume_cursor(&api), first.value);
        EXPECT(cursor.get_text() == "public"_view);
        EXPECT_EQ(api.index, U64(1));
        EXPECT(Source::Lexical::Cursor(api).get_text() == "f"_view);

        output.get_query().bind<Source::Content>().visit(
            [&](Source::Content content) {
              Flow flow;
              ASSERT(
                  flow.connect(Flow::reader(form), content.get_data()) ==
                  Flow::Status::Success);
              flow.visit(
                  [&](const void* data) { EXPECT(data == text.get_data()); },
                  [&](const void*) { EXPECT(False); },
                  [&](auto, auto) { EXPECT(False); },
                  [&](auto) { EXPECT(False); });
            },
            [&](Binding::Failure) { EXPECT(False); });
      },
      [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(LexicalPublication, foreign_materialization) {
  U32 releases = 0;
  Publication input(
      source_content_open(&form, 8, 'a', TTX_BINDING_SATISFIED, 0, &releases));
  input.get_query().bind<Abstract>().visit(
      [&](Abstract source) {
        tokenize(source).visit(
            [&](Publication& output) {
              EXPECT_EQ(source_content_reads(input.get_query()), U32(1));
              output.get_query().bind<Source::Lexical::Cursor>().visit(
                  [&](Source::Lexical::Cursor tokens) {
                    EXPECT(
                        tokens.get_anchor(tokens.current()).get_source() ==
                        source);
                    EXPECT(
                        tokens.peek(1).get_code() ==
                        Source::Lexical::Code::Type::Terminal);
                    EXPECT_EQ(tokens.get_text().get_size(), Count(8));
                  },
                  [&](Binding::Failure) { EXPECT(False); });

              // A dialect can inspect spelling repeatedly without reobserving
              // the remote producer. The lexical publication owns this copy.
              output.get_query().bind<Source::Content>().visit(
                  [&](Source::Content content) {
                    Flow flow;
                    ASSERT(
                        flow.connect(Flow::reader(form), content.get_data()) ==
                        Flow::Status::Success);
                    EXPECT(flow.get_protocol() == Flow::Protocol::Direct);
                    U8 text[8];
                    const Storage target(
                        ttx_storage{&form, text, sizeof(text)});
                    for (Count i = 0; i < 3; ++i) {
                      EXPECT(
                          Ttx::Semantic::Flows::Copy::flow(flow, target) ==
                          Ttx::Data::Status::Success);
                      EXPECT(
                          Core::View::Bytes(text, sizeof(text)) ==
                          "abcdefgh"_view);
                    }
                  },
                  [&](Binding::Failure) { EXPECT(False); });
              EXPECT_EQ(source_content_reads(input.get_query()), U32(1));
            },
            [&](Binding::Failure) { EXPECT(False); });
      },
      [&](Binding::Failure) { EXPECT(False); });
  EXPECT_EQ(releases, U32(0));
  input.close();
  EXPECT_EQ(releases, U32(1));
}

struct SharedInput {
  mutable Count acquisitions = 0;
  mutable Count releases = 0;
  auto get_data() const -> Core::View::Bytes { return {}; }
  auto supports(System::Uuid id) const -> Binding::Status {
    return id == Source::Content::contract_id ||
                   id == Shared::Access::contract_id
               ? Binding::Status::Satisfied
               : Binding::Status::Unsupported;
  }
  auto bind_interface(System::Uuid id, Storage target) const
      -> Binding::Status {
    if (id == Source::Content::contract_id) {
      const Source::Content::Api api = {
        this, [](const void*) -> U64 { return 8; },
        [](const void* self) -> ttx_semantic_query {
          return Abstract::provide(*static_cast<const SharedInput*>(self))
              .get_query();
        }};
      return Binding::provide<Source::Content>(api, target);
    }

    if (id == Shared::Access::contract_id) {
      static const Shared::Access::Operations operations = {
        [](const void*) -> const ttx_representation* { return &form; },
        [](const void* self, ttx_shared_lifetime* lifetime) -> ttx_data_status {
          ++static_cast<const SharedInput*>(self)->acquisitions;
          *lifetime = {"public f", self, [](const void* value) {
                         ++static_cast<const SharedInput*>(value)->releases;
                       }};
          return TTX_DATA_SUCCESS;
        }};
      return Binding::provide<Shared::Access>(
          Shared::Access::Api(this, &operations), target);
    }

    return Binding::Status::Unsupported;
  }
};

PERIMORTEM_UNIT_TEST(LexicalPublication, shared_lifetime) {
  const SharedInput input;
  tokenize(Abstract::provide(input))
      .visit(
          [&](Publication& output) {
            EXPECT_EQ(input.acquisitions, Count(1));
            EXPECT_EQ(input.releases, Count(0));
            Publication moved(static_cast<Publication&&>(output));
            output.close();
            EXPECT_EQ(input.releases, Count(0));
            moved.get_query().bind<Source::Lexical::Cursor>().visit(
                [&](Source::Lexical::Cursor tokens) {
                  EXPECT(
                      tokens.peek(2).get_code() ==
                      Source::Lexical::Code::Type::Terminal);
                },
                [&](Binding::Failure) { EXPECT(False); });
          },
          [&](Binding::Failure) { EXPECT(False); });
  EXPECT_EQ(input.releases, Count(1));
}

PERIMORTEM_UNIT_TEST(LexicalPublication, empty_input) {
  const Source::Contents::Memory input(Core::View::Bytes(), empty_form);
  tokenize(Abstract::provide(input))
      .visit(
          [&](Publication& output) {
            output.get_query().bind<Source::Lexical::Cursor>().visit(
                [&](Source::Lexical::Cursor tokens) {
                  EXPECT(
                      tokens.current().get_code() ==
                      Source::Lexical::Code::Type::Terminal);
                  EXPECT_EQ(
                      tokens.get_anchor(tokens.current())
                          .get_extent()
                          .get_offset(),
                      U64(0));
                },
                [&](Binding::Failure) { EXPECT(False); });
          },
          [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(LexicalPublication, failed_input) {
  U32 releases = 0;
  const ttx_binding_status statuses[] = {
    TTX_BINDING_PENDING, TTX_BINDING_REJECTED, TTX_BINDING_UNSUPPORTED,
    TTX_BINDING_SATISFIED};
  for (const auto status : statuses) {
    Publication input(source_content_open(&form, 8, 'a', status, 1, &releases));
    input.get_query().bind<Abstract>().visit(
        [&](Abstract source) {
          tokenize(source).visit(
              [&](Publication&) { EXPECT(False); },
              [&](Binding::Failure failure) {
                const auto expected = status == TTX_BINDING_SATISFIED
                                          ? TTX_BINDING_REJECTED
                                          : status;
                EXPECT(failure == Binding::Failure(expected));
              });
        },
        [&](Binding::Failure) { EXPECT(False); });
  }

  EXPECT_EQ(releases, U32(4));
}

struct DifferentCursorOps {
  void (*get_token)(const void*, U64, tetrodotoxin_source_token*);
  decltype(tetrodotoxin_source_cursor_ops::get_text) get_text;
  decltype(tetrodotoxin_source_cursor_ops::get_anchor) get_anchor;
  decltype(tetrodotoxin_source_cursor_ops::get_error_count) get_error_count;
  decltype(tetrodotoxin_source_cursor_ops::report) report;
  decltype(tetrodotoxin_source_cursor_ops::get_error) get_error;
};
TTX_DATA_RECORD(
    DifferentCursorOps,
    TTX_DATA_MEMBER(DifferentCursorOps, get_token),
    TTX_DATA_MEMBER(DifferentCursorOps, get_text),
    TTX_DATA_MEMBER(DifferentCursorOps, get_anchor),
    TTX_DATA_MEMBER(DifferentCursorOps, get_error_count),
    TTX_DATA_MEMBER(DifferentCursorOps, report),
    TTX_DATA_MEMBER(DifferentCursorOps, get_error));

struct DifferentCursor {
  const void* source;
  const DifferentCursorOps* operations;
  U64 index;
};
TTX_DATA_RECORD(
    DifferentCursor,
    TTX_DATA_MEMBER(DifferentCursor, source),
    TTX_DATA_MEMBER(DifferentCursor, operations),
    TTX_DATA_MEMBER(DifferentCursor, index));

PERIMORTEM_UNIT_TEST(LexicalPublication, cursor_abi_mismatch) {
  const Source::Contents::Memory input("public f"_view, form);
  tokenize(Abstract::provide(input))
      .visit(
          [&](Publication& output) {
            // Both outer records contain two pointers and an index, and both
            // operation tables have equal size. The output-Token signature
            // behind one table pointer must still prevent binding the new
            // contract.
            static_assert(
                sizeof(DifferentCursor) == sizeof(tetrodotoxin_source_cursor));
            static_assert(
                sizeof(DifferentCursorOps) ==
                sizeof(tetrodotoxin_source_cursor_ops));
            const auto& different = Compiled<
                Native<DifferentCursor>::reference>::get_representation();
            DifferentCursor api = {};
            const Storage destination(
                ttx_storage{
                  &different, reinterpret_cast<U8*>(&api), sizeof(api)});
            EXPECT(
                output.get_query().bind(
                    Source::Lexical::Cursor::contract_id, destination) ==
                Binding::Status::Rejected);
            EXPECT(api.source == nullptr);
            EXPECT(api.operations == nullptr);
          },
          [&](Binding::Failure) { EXPECT(False); });
}
