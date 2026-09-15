// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/modules/module.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/answers/constant.hpp"
#include "ttx/concept/answers/none.hpp"

using namespace Perimortem;
using Ttx::Concept::Modules::Module;
using namespace Ttx::Semantic::Negotiation;

static Validation::Harness Modules = {.name = "TTX::Concept::Module"_view};

// The encountered policy rejects a question which its resolved referent could
// answer. Acquisition must expose that policy directly instead of choosing a
// more permissive root while adapting the lifetime carrier.
struct Policy {
  mutable Count bindings = 0;
  auto get_data() const -> Core::View::Bytes { return "policy"_view; }
  auto resolve() const -> Ttx::Concept::Abstract {
    return Ttx::Concept::Answers::None::get_none();
  }
  auto bind_interface(System::Uuid, Ttx::Data::Form::Storage) const
      -> Binding::Status {
    ++bindings;
    return Binding::Status::Rejected;
  }
};

struct Provider {
  Count references = 1;
  Policy root;
  Count opened = 0;
  Count closed = 0;
  const void* host = nullptr;
  Ttx::Data::Status status = Ttx::Data::Status::Success;
  bool malformed = false;

  auto get_module() -> ttx_module {
    return {
      this,
      [](const void* source) {
        ++const_cast<Provider*>(static_cast<const Provider*>(source))
              ->references;
      },
      [](const void* source) {
        --const_cast<Provider*>(static_cast<const Provider*>(source))
              ->references;
      },
      [](const void* source, ttx_semantic_query host,
         ttx_module_acquisition* output) -> ttx_data_status {
        auto& provider =
            *const_cast<Provider*>(static_cast<const Provider*>(source));
        provider.host = host.source;
        if (provider.status != Ttx::Data::Status::Success) {
          return static_cast<ttx_data_status>(provider.status);
        }

        ++provider.opened;
        *output = {
          Ttx::Concept::Abstract::provide(provider.root).get_abi(), &provider,
          [](const void* owner) {
            ++const_cast<Provider*>(static_cast<const Provider*>(owner))
                  ->closed;
          }};
        if (provider.malformed) {
          output->root.operations = nullptr;
        }
        return TTX_DATA_SUCCESS;
      }};
  }
};

PERIMORTEM_UNIT_TEST(Modules, direct_policy) {
  Provider provider;
  {
    Module module(provider.get_module());
    U8 service = 0;
    const Ttx::Semantic::Negotiation::Query host(
        {&service,
         [](const void*, perimortem_uuid, ttx_storage) -> ttx_binding_status {
           return TTX_BINDING_UNSUPPORTED;
         }});
    module.open(host).visit(
        [&](Module::Acquisition& acquired) {
          EXPECT(provider.host == &service);
          EXPECT_EQ(provider.root.bindings, Count(0));
          EXPECT(acquired.get_data() == "policy"_view);
          acquired.bind<Ttx::Concept::Answers::Constant>().visit(
              [&](Ttx::Concept::Answers::Constant) { EXPECT(False); },
              [&](Binding::Failure failure) {
                EXPECT(failure == Binding::Failure::Rejected);
              });
          acquired.resolve().bind<Ttx::Concept::Answers::Constant>().visit(
              [](Ttx::Concept::Answers::Constant) {},
              [&](Binding::Failure) { EXPECT(False); });

          // Moving and exporting the owner must preserve the interior root.
          // Release uses its independent owner pointer, not the root receiver.
          Module::Acquisition moved(Core::Data::take(acquired));
          acquired.close();
          EXPECT_EQ(provider.closed, Count(0));
          const auto exported = moved.take();
          EXPECT(exported.root.source == &provider.root);
          EXPECT(exported.owner == &provider);
          EXPECT(exported.owner != exported.root.source);
          exported.release(exported.owner);
          moved.close();
          EXPECT_EQ(provider.closed, Count(1));
        },
        [&](Ttx::Data::Status) { EXPECT(False); });
    EXPECT_EQ(provider.references, Count(1));
  }
  EXPECT_EQ(provider.references, Count(0));
  EXPECT_EQ(provider.opened, provider.closed);
}

PERIMORTEM_UNIT_TEST(Modules, acquisition_failure) {
  Provider provider;
  Module module(provider.get_module());
  provider.status = Ttx::Data::Status::Denied;
  module.open().visit(
      [&](Module::Acquisition&) { EXPECT(False); },
      [&](Ttx::Data::Status status) {
        EXPECT(status == Ttx::Data::Status::Denied);
      });
  EXPECT_EQ(provider.opened, Count(0));
  EXPECT_EQ(provider.closed, Count(0));

  // A successful entry has transferred a release obligation even if it fails
  // to supply a usable root. Admission returns that obligation to its actual
  // owner before reporting the malformed result, with no root callbacks.
  provider.status = Ttx::Data::Status::Success;
  provider.malformed = true;
  module.open().visit(
      [&](Module::Acquisition&) { EXPECT(False); },
      [&](Ttx::Data::Status status) {
        EXPECT(status == Ttx::Data::Status::Invalid);
      });
  EXPECT_EQ(provider.closed, Count(1));
  EXPECT_EQ(provider.root.bindings, Count(0));

  provider.malformed = false;
  {
    Module retained(module);
    EXPECT_EQ(provider.references, Count(2));
    retained.open().visit(
        [&](Module::Acquisition& acquired) {
          EXPECT(acquired.get_data() == "policy"_view);
        },
        [&](Ttx::Data::Status) { EXPECT(False); });
  }
  EXPECT_EQ(provider.references, Count(1));
  EXPECT_EQ(provider.opened, provider.closed);
}
