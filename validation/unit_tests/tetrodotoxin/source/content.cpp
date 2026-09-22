// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/tetrodotoxin/source/content.h"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/source/contents/memory.hpp"
#include "tetrodotoxin/source/policies/authored.hpp"
#include "ttx/concept/answers/none.hpp"
#include "ttx/semantic/flows/copy.hpp"
#include "ttx/semantic/ownership/publication.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Ttx::Concept;
using namespace Ttx::Data::Form;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Semantic::Ownership;
using namespace Ttx::Semantic::Transport;
using namespace Ttx::Semantic::Flows;

static Validation::Harness SourceContent = {.name = "Source::Content"_view};
static constexpr auto bytes = Schema::range(Native<U8>::reference, 8, 1, 8, 1);
static constexpr auto empty = Schema::range(Native<U8>::reference, 0, 1, 0, 1);
static constexpr auto& form = Compiled<bytes>::get_representation();
static constexpr auto& empty_form = Compiled<empty>::get_representation();

PERIMORTEM_UNIT_TEST(SourceContent, direct_observation) {
  const Contents::Memory observation("abcdefgh"_view, form);
  const auto source = Abstract::provide(observation);
  source.bind<Content>().visit(
      [&](Content content) {
        EXPECT_EQ(content.get_size(), Count(8));
        Flow flow;
        ASSERT(
            flow.connect(Flow::reader(form), content.get_data()) ==
            Flow::Status::Success);
        EXPECT(flow.get_protocol() == Flow::Protocol::Direct);
        U8 output[8] = {};
        const Storage storage(ttx_storage{&form, output, sizeof(output)});
        EXPECT(Copy::flow(flow, storage) == Ttx::Data::Status::Success);
        EXPECT(Core::View::Bytes(output, sizeof(output)) == "abcdefgh"_view);
      },
      [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(SourceContent, foreign_observation) {
  U32 releases = 0;
  {
    Publication publication(source_content_open(
        &form, 8, 'a', TTX_BINDING_SATISFIED, 0, &releases));
    publication.get_query().bind<Content>().visit(
        [&](Content content) {
          // This provider never lends a pointer. The same consumer chooses
          // Block and asks it to populate the caller's independent storage.
          Flow flow;
          ASSERT(
              flow.connect(Flow::reader(form), content.get_data()) ==
              Flow::Status::Success);
          EXPECT(flow.get_protocol() == Flow::Protocol::Block);
          U8 output[8] = {};
          const Storage storage(ttx_storage{&form, output, sizeof(output)});
          EXPECT(Copy::flow(flow, storage) == Ttx::Data::Status::Success);
          EXPECT(Core::View::Bytes(output, sizeof(output)) == "abcdefgh"_view);
        },
        [&](Binding::Failure) { EXPECT(False); });
    EXPECT_EQ(releases, U32(0));
  }

  EXPECT_EQ(releases, U32(1));
}

PERIMORTEM_UNIT_TEST(SourceContent, retained_revision) {
  U32 releases = 0;
  Publication old(
      source_content_open(&form, 8, 'a', TTX_BINDING_SATISFIED, 0, &releases));
  Publication replacement(
      source_content_open(&form, 8, 'm', TTX_BINDING_SATISFIED, 0, &releases));
  old.get_query().bind<Abstract>().visit(
      [&](Abstract source) {
        Policies::Authored declaration(
            Answers::None::get_none(), Anchor(source, Range(1, 2)));
        // Replacing the host's current publication does not retarget existing
        // graph edges. The declaration still reaches the original C provider.
        replacement.close();
        EXPECT_EQ(releases, U32(1));
        Abstract::provide(declaration)
            .bind<Declaration>()
            .visit(
                [&](Declaration facts) {
                  ASSERT(facts.get_anchor());
                  facts.get_anchor()->get_source().bind<Content>().visit(
                      [&](Content content) {
                        Flow flow;
                        ASSERT(
                            flow.connect(
                                Flow::reader(form), content.get_data()) ==
                            Flow::Status::Success);
                        U8 output[8] = {};
                        EXPECT(
                            Copy::flow(
                                flow, Storage(
                                          ttx_storage{
                                            &form, output, sizeof(output)})) ==
                            Ttx::Data::Status::Success);
                        EXPECT(
                            Core::View::Bytes(output, sizeof(output)) ==
                            "abcdefgh"_view);
                      },
                      [&](Binding::Failure) { EXPECT(False); });
                },
                [&](Binding::Failure) { EXPECT(False); });
      },
      [&](Binding::Failure) { EXPECT(False); });
  old.close();
  EXPECT_EQ(releases, U32(2));
}

PERIMORTEM_UNIT_TEST(SourceContent, empty_observation) {
  const Contents::Memory observation(Core::View::Bytes(), empty_form);
  const auto source = Abstract::provide(observation);
  const Anchor caret(source, Range(0, 0), Range(0, 0));
  EXPECT(caret.get_focus());
  source.bind<Content>().visit(
      [&](Content content) {
        EXPECT_EQ(content.get_size(), Count(0));
        Flow flow;
        ASSERT(
            flow.connect(Flow::reader(empty_form), content.get_data()) ==
            Flow::Status::Success);
        EXPECT(
            Copy::flow(flow, Storage(ttx_storage{&empty_form, nullptr, 0})) ==
            Ttx::Data::Status::Success);
      },
      [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(SourceContent, policy_outcomes) {
  U32 releases = 0;
  const ttx_binding_status statuses[] = {
    TTX_BINDING_PENDING, TTX_BINDING_REJECTED, TTX_BINDING_UNSUPPORTED};
  for (const auto status : statuses) {
    Publication publication(
        source_content_open(&form, 8, 'a', status, 0, &releases));
    EXPECT(
        publication.get_query().supports<Content>() == Binding::Status(status));
    publication.get_query().bind<Content>().visit(
        [&](Content) { EXPECT(False); },
        [&](Binding::Failure failure) {
          EXPECT(failure == Binding::Failure(status));
        });
  }

  EXPECT_EQ(releases, U32(3));
}

PERIMORTEM_UNIT_TEST(SourceContent, failed_observation) {
  U32 releases = 0;
  Publication publication(
      source_content_open(&form, 8, 'a', TTX_BINDING_SATISFIED, 1, &releases));
  publication.get_query().bind<Content>().visit(
      [&](Content content) {
        Flow flow;
        ASSERT(
            flow.connect(Flow::reader(form), content.get_data()) ==
            Flow::Status::Success);
        U8 output[8] = {};
        EXPECT(
            Copy::flow(
                flow, Storage(ttx_storage{&form, output, sizeof(output)})) ==
            Ttx::Data::Status::IoError);
      },
      [&](Binding::Failure) { EXPECT(False); });
}

PERIMORTEM_UNIT_TEST(SourceContent, incompatible_bytes) {
  const Contents::Memory observation("abcdefgh"_view, form);
  Abstract::provide(observation)
      .bind<Content>()
      .visit(
          [&](Content content) {
            // Equal extents do not make two U32 values a byte stream. Source
            // leaves wire agreement to Flow rather than inventing a cast here.
            const auto& words =
                Compiled<Native<U32[2]>::reference>::get_representation();
            Flow flow;
            EXPECT(
                flow.connect(Flow::reader(words), content.get_data()) ==
                Flow::Status::Incompatible);
          },
          [&](Binding::Failure) { EXPECT(False); });
}
