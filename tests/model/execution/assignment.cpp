// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/model/execution/assignments/memory.hpp"
#include "tetrodotoxin/model/execution/values/literal.hpp"
#include "tetrodotoxin/model/type/primitives/u32.hpp"
#include "toolchain/validation/unit_test.hpp"
#include "ttx/data/protocol/block/provider.hpp"
#include "ttx/semantic/transport/flow.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Model;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;

static Toolchain::Validation::Harness Types = {
  .name = "Model::Execution::Assignment"};

// Binding while a private or compile time authority allows writing cannot
// grant permanent permission. This Addressable style policy checks authority
// inside each invocation before reaching the concrete memory destination.
VALIDATION_TEST(Types, live_write_authority) {
  const Type::Primitives::U32 type;
  U32 stored = 11;
  const auto& form = type.get_representation();
  const Execution::Assignments::Memory memory(
      Abstract::provide(type),
      Ttx::Data::Form::Storage(
          ttx_storage{&form, reinterpret_cast<U8*>(&stored), sizeof(stored)}));
  struct Policy {
    Abstract target;
    Binding::Status authority = Binding::Status::Satisfied;
    bool constant_only = false;
    auto get_data() const -> Core::View::Bytes { return {}; }
    auto supports(System::Uuid id) const -> Binding::Status {
      return id == Execution::Assignment::contract_id ? authority
                                                      : target.supports(id);
    }
    auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage output) const
        -> Binding::Status {
      if (id != Execution::Assignment::contract_id) {
        return target.bind_interface(id, output);
      }

      if (authority != Binding::Status::Satisfied) {
        return authority;
      }

      const Execution::Assignment::Api api = {
        this,
        [](const void* self, ttx_abstract value,
           ttx_data_status* output) -> ttx_binding_status {
          const auto& policy = *static_cast<const Policy*>(self);
          if (policy.authority != Binding::Status::Satisfied) {
            return static_cast<ttx_binding_status>(policy.authority);
          }

          if (policy.constant_only) {
            const auto status = Abstract(value).supports<Execution::Constant>();
            if (status != Binding::Status::Satisfied) {
              return static_cast<ttx_binding_status>(status);
            }
          }

          return policy.target.bind<Execution::Assignment>().visit(
              [&](Execution::Assignment assignment) -> ttx_binding_status {
                return assignment.assign(Abstract(value))
                    .visit(
                        [&](Ttx::Data::Status status) -> ttx_binding_status {
                          *output = static_cast<ttx_data_status>(status);
                          return TTX_BINDING_SATISFIED;
                        },
                        [](Binding::Failure failure) {
                          return static_cast<ttx_binding_status>(failure);
                        });
              },
              [](Binding::Failure failure) {
                return static_cast<ttx_binding_status>(failure);
              });
        }};
      return Binding::provide<Execution::Assignment>(api, output);
    }
  } policy(Abstract::provide(memory));
  const Execution::Values::Literal<U32> value(Abstract::provide(type), 42);
  Abstract::provide(policy).bind<Execution::Assignment>().visit(
      [&](Execution::Assignment assignment) {
        const Binding::Status statuses[] = {
          Binding::Status::Rejected, Binding::Status::Unknown};
        for (const auto status : statuses) {
          policy.authority = status;
          assignment.assign(Abstract::provide(value))
              .visit(
                  [&](Ttx::Data::Status) { EXPECT(False); },
                  [&](Binding::Failure failure) {
                    EXPECT(static_cast<Binding::Status>(failure) == status);
                  });
          EXPECT_EQ(stored, U32(11));
        }

        policy.authority = Binding::Status::Satisfied;
        assignment.assign(Abstract::provide(value))
            .visit(
                [&](Ttx::Data::Status status) {
                  EXPECT(status == Ttx::Data::Status::Success);
                },
                [&](Binding::Failure) { EXPECT(False); });
        EXPECT_EQ(stored, U32(42));

        // The location supplies no Constant promise. That leaves admission
        // Unknown even though its Type and representation match the literal.
        policy.constant_only = true;
        assignment.assign(Abstract::provide(memory))
            .visit(
                [&](Ttx::Data::Status) { EXPECT(False); },
                [&](Binding::Failure failure) {
                  EXPECT(failure == Binding::Failure::Unknown);
                });
        assignment.assign(Abstract::provide(value))
            .visit(
                [&](Ttx::Data::Status status) {
                  EXPECT(status == Ttx::Data::Status::Success);
                },
                [&](Binding::Failure) { EXPECT(False); });
      },
      [&](Binding::Failure) { EXPECT(False); });
}

// The Type admits this value, but its Block provider can fail after touching
// the output. The assignment result must preserve that transfer outcome rather
// than report a type rejection or imply transactional rollback.
VALIDATION_TEST(Types, transfer_failure) {
  const Type::Primitives::U32 type;
  const Execution::Values::Literal<U32> literal(Abstract::provide(type), 19);
  struct Source {
    Abstract target;
    const Ttx::Data::Form::Representation& form;
    mutable Count writes = 0;
    Binding::Status permission = Binding::Status::Satisfied;
    auto get_data() const -> Core::View::Bytes { return {}; }
    auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage output) const
        -> Binding::Status {
      if (id == Execution::Value::contract_id) {
        const Execution::Value::Api api = {
          this, [](const void* self) -> ttx_semantic_query {
            return Abstract::provide(*static_cast<const Source*>(self))
                .get_query();
          }};
        return Binding::provide<Execution::Value>(api, output);
      }

      if (id == Ttx::Semantic::Transport::Flow::block.provider) {
        if (permission != Binding::Status::Satisfied) {
          return permission;
        }

        static const Ttx::Data::Protocol::Block::Provider::Operations
            operations = {
              [](const void* self) -> const ttx_representation* {
                return &static_cast<const Source*>(self)->form;
              },
              [](const void* self, ttx_storage surface) -> ttx_data_status {
                ++static_cast<const Source*>(self)->writes;
                surface.data[0] = 27;
                return TTX_DATA_IO_ERROR;
              }};
        return Binding::provide<Ttx::Data::Protocol::Block::Provider>(
            Ttx::Data::Protocol::Block::Provider::Api(this, &operations),
            output);
      }

      if (id == Ttx::Concept::Domain::contract_id) {
        return target.bind_interface(id, output);
      }

      return Binding::Status::Unknown;
    }
  } source(Abstract::provide(literal), type.get_representation());
  U32 output = 0;
  const Execution::Assignments::Memory memory(
      Abstract::provide(type),
      Ttx::Data::Form::Storage(ttx_storage(
          &source.form, reinterpret_cast<U8*>(&output), sizeof(output))));
  memory.assign(Abstract::provide(source))
      .visit(
          [&](Ttx::Data::Status status) {
            EXPECT(status == Ttx::Data::Status::IoError);
          },
          [&](Binding::Failure) { EXPECT(False); });
  EXPECT_EQ(source.writes, Count(1));
  EXPECT_EQ(output, U32(27));

  output = 11;
  source.permission = Binding::Status::Rejected;
  memory.assign(Abstract::provide(source))
      .visit(
          [&](Ttx::Data::Status) { EXPECT(False); },
          [&](Binding::Failure failure) {
            EXPECT(failure == Binding::Failure::Rejected);
          });
  EXPECT_EQ(source.writes, Count(1));
  EXPECT_EQ(output, U32(11));
}
