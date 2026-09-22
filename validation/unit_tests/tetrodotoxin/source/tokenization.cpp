// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/tokenization.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/source/contents/memory.hpp"
#include "validation/unit_tests/tetrodotoxin/source/content.h"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Ttx::Concept;
using namespace Ttx::Data::Form;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Semantic::Ownership;

static Validation::Harness SourceTokenization = {
  .name = "Source::Tokenization"_view};
static constexpr auto bytes = Schema::range(Native<U8>::reference, 8, 1, 8, 1);
static constexpr auto& form = Compiled<bytes>::get_representation();

struct Service {
  Abstract observed;
  Binding::Status status = Binding::Status::Satisfied;
  U32 calls = 0;
  U32 releases = 0;

  explicit Service(Abstract source) : observed(source) {}

  auto get_api() -> Tokenization::Api {
    return {
      this,
      [](const void* self, ttx_abstract source,
         ttx_publication* output) -> ttx_binding_status {
        auto& service =
            *const_cast<Service*>(static_cast<const Service*>(self));
        service.observed = Abstract(source);
        ++service.calls;
        if (service.status != Binding::Status::Satisfied) {
          return static_cast<ttx_binding_status>(service.status);
        }

        *output = source_content_open(
            &form, 8, 'a', TTX_BINDING_SATISFIED, 0, &service.releases);
        return TTX_BINDING_SATISFIED;
      }};
  }
};

PERIMORTEM_UNIT_TEST(SourceTokenization, publication_boundary) {
  const Contents::Memory input("abcdefgh"_view, form);
  const auto source = Abstract::provide(input);
  Service service(source);
  const auto api = service.get_api();
  const Query query(source_tokenization_query(&api));

  // The C entry publishes the complete Tokenization record. This synthetic
  // frontend exposes Content as its result vocabulary, proving that the common
  // operation does not require the native TTX Token or Cursor representation.
  query.bind<Tokenization>().visit(
      [&](Tokenization tokenizer) {
        tokenizer.tokenize(source).visit(
            [&](Publication& result_owner) {
              EXPECT(service.observed == source);
              EXPECT_EQ(service.calls, U32(1));
              Publication retained(static_cast<Publication&&>(result_owner));
              result_owner.close();
              EXPECT_EQ(service.releases, U32(0));
              retained.get_query().bind<Content>().visit(
                  [&](Content content) {
                    EXPECT_EQ(content.get_size(), Count(8));
                  },
                  [&](Binding::Failure) { EXPECT(False); });
            },
            [&](Binding::Failure) { EXPECT(False); });
      },
      [&](Binding::Failure) { EXPECT(False); });

  EXPECT_EQ(service.releases, U32(1));
}

PERIMORTEM_UNIT_TEST(SourceTokenization, unsettled_result) {
  const Contents::Memory input("abcdefgh"_view, form);
  const auto source = Abstract::provide(input);
  Service service(source);
  const auto api = service.get_api();
  const Query query(source_tokenization_query(&api));

  // Obtaining the service is separate from accepting a particular input.
  // Failed operations create no output owner and schedule no later callback.
  query.bind<Tokenization>().visit(
      [&](Tokenization tokenizer) {
        const Binding::Status statuses[] = {
          Binding::Status::Pending, Binding::Status::Rejected,
          Binding::Status::Unsupported};
        for (const auto status : statuses) {
          service.status = status;
          tokenizer.tokenize(source).visit(
              [&](Publication&) { EXPECT(False); },
              [&](Binding::Failure failure) {
                EXPECT(failure == static_cast<Binding::Failure>(status));
              });
        }
      },
      [&](Binding::Failure) { EXPECT(False); });

  EXPECT_EQ(service.calls, U32(3));
  EXPECT_EQ(service.releases, U32(0));
}
