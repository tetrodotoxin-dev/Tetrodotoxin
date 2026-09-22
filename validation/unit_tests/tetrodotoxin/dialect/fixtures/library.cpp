// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <assert.h>

#include "tetrodotoxin/dialect/library/dialect.hpp"
#include "ttx/concept/modules/module.hpp"
#include "validation/unit_tests/tetrodotoxin/dialect/fixtures/lifetime.h"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Semantic::Ownership;

static monograph_lifetime* lifetime;
static const Dialect::Library::Dialect language(
    System::Uuid(0x01a201fea3b44861, 0xa3f996cd34ae0c32));

// Instrument release without changing the Abstract returned by the real
// Library invocation. Its Query still binds the original Monograph policy.
class ObservedMonograph {
 public:
  explicit ObservedMonograph(Publication&& child)
      : child(Core::Data::take(child)) {
    ++lifetime->live;
  }
  ~ObservedMonograph() {
    child.close();
    --lifetime->live;
    ++lifetime->releases;
  }
  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage target) const
      -> Binding::Status {
    return child.get_query().bind(id, target);
  }
  auto supports(System::Uuid id) const -> Binding::Status {
    return child.get_query().supports(id);
  }
  Publication child;
};

static auto interpret(
    const void*,
    tetrodotoxin_source_cursor* cursor,
    ttx_abstract context,
    ttx_publication* output) -> ttx_binding_status {
  return Abstract::provide(language).bind<Source::Dialect>().visit(
      [&](Source::Dialect dialect) -> ttx_binding_status {
        Source::Lexical::Cursor input(*cursor);
        auto result = dialect.interpret(input, Abstract(context));
        cursor->index = input.get_index();
        return result.visit(
            [&](Publication& child) -> ttx_binding_status {
              auto* owner = new ObservedMonograph(Core::Data::take(child));
              *output = {
                {owner,
                 [](const void* self, perimortem_uuid id, ttx_storage target) {
                   return static_cast<ttx_binding_status>(
                       static_cast<const ObservedMonograph*>(self)
                           ->bind_interface(
                               System::Uuid(id),
                               Ttx::Data::Form::Storage(target)));
                 },
                 [](const void* self, perimortem_uuid id) {
                   return static_cast<ttx_binding_status>(
                       static_cast<const ObservedMonograph*>(self)->supports(
                           System::Uuid(id)));
                 }},
                [](const void* self) {
                  delete static_cast<const ObservedMonograph*>(self);
                }};
              return TTX_BINDING_SATISFIED;
            },
            [](Binding::Failure failure) {
              return static_cast<ttx_binding_status>(failure);
            });
      },
      [](Binding::Failure failure) {
        return static_cast<ttx_binding_status>(failure);
      });
}

struct LibraryModuleFixture {
  auto get_data() const -> Core::View::Bytes { return {}; }
  auto supports(System::Uuid id) const -> Binding::Status {
    return id == Source::Dialect::contract_id ? Binding::Status::Satisfied
                                              : Binding::Status::Unsupported;
  }
  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage target) const
      -> Binding::Status {
    if (id != Source::Dialect::contract_id) {
      return Binding::Status::Unsupported;
    }

    return Binding::provide<Source::Dialect>(
        Source::Dialect::Api{this, interpret}, target);
  }
};
static const LibraryModuleFixture module;

auto monograph_fixture_watch(monograph_lifetime* value) -> void {
  lifetime = value;
}

PERIMORTEM_C auto ttx_module_open(
    ttx_semantic_query,
    ttx_module_acquisition* output) -> ttx_data_status {
  ++lifetime->acquisitions;
  *output = {Abstract::provide(module).get_abi(), &module, [](const void*) {
               ++lifetime->acquisition_releases;
             }};
  return TTX_DATA_SUCCESS;
}

// An unloaded provider can no longer run any result finalizer. Verify that
// every invocation and discovery acquisition has ended before that happens.
__attribute__((destructor)) static void unloaded() {
  if (lifetime) {
    assert(lifetime->live == 0);
    assert(lifetime->acquisitions == lifetime->acquisition_releases);
    ++lifetime->unloaded;
  }
}
