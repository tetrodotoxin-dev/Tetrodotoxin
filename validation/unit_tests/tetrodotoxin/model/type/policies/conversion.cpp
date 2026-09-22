// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/model/execution/assignments/memory.hpp"
#include "tetrodotoxin/model/execution/values/literal.hpp"
#include "tetrodotoxin/model/type/primitives/u32.hpp"
#include "tetrodotoxin/model/type/primitives/u8.hpp"
#include "ttx/semantic/flows/copy.hpp"
#include "ttx/semantic/transport/block.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Model;
using namespace Ttx::Concept;
using namespace Ttx::Semantic::Negotiation;

static Validation::Harness Types = {.name = "Model::Type::Conversion"_view};

// Retaining a conversion interface retains its callable lifetime only. The
// source's Type policy changes after the first call, so the same interface
// must observe Pending, rejection and the later successful answer in turn.
PERIMORTEM_UNIT_TEST(Types, live_conversion) {
  const Type::Primitives::U32 base;
  struct Policy {
    Abstract subject;
    Binding::Status status = Binding::Status::Satisfied;
    mutable Count observations = 0;
    auto get_data() const -> Core::View::Bytes { return {}; }
    auto supports(System::Uuid id) const -> Binding::Status {
      ++observations;
      return status == Binding::Status::Satisfied ? subject.supports(id)
                                                  : status;
    }
    auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage output) const
        -> Binding::Status {
      return status == Binding::Status::Satisfied
                 ? subject.bind_interface(id, output)
                 : status;
    }
  } policy(Abstract::provide(base));
  const Execution::Values::Literal<U32> value(Abstract::provide(policy), 7);
  Abstract::provide(base).bind<Type::Policies::Conversion>().visit(
      [&](Type::Policies::Conversion conversion) {
        conversion.convert(Abstract::provide(value))
            .visit(
                [&](Abstract output) {
                  EXPECT(output == Abstract::provide(value));
                },
                [&](Binding::Failure) { EXPECT(False); });
        const Binding::Status statuses[] = {
          Binding::Status::Pending, Binding::Status::Rejected,
          Binding::Status::Unsupported};
        for (const auto status : statuses) {
          policy.status = status;
          conversion.convert(Abstract::provide(value))
              .visit(
                  [&](Abstract) { EXPECT(False); },
                  [&](Binding::Failure failure) {
                    EXPECT(static_cast<Binding::Status>(failure) == status);
                  });
        }

        policy.status = Binding::Status::Satisfied;
        conversion.convert(Abstract::provide(value))
            .visit(
                [&](Abstract output) {
                  EXPECT(output == Abstract::provide(value));
                },
                [&](Binding::Failure) { EXPECT(False); });
        EXPECT_EQ(policy.observations, Count(5));
      },
      [&](Binding::Failure) { EXPECT(False); });
}

// The converted subject keeps one observed byte, then supplies its wider public
// form through Block. Its private storage never needs to resemble that form.
// This is an execution value supplied by a policy, not a Data conversion rule.
class Widened {
 public:
  Widened(Abstract type, U8 observation)
      : type(type), observation(observation) {}
  auto get_data() const -> Core::View::Bytes { return {}; }
  auto supports(System::Uuid id) const -> Binding::Status {
    return id == Ttx::Concept::Domain::contract_id ||
                   id == Execution::Value::contract_id ||
                   id == Execution::Constant::contract_id ||
                   id == Ttx::Semantic::Transport::Block::Access::contract_id
               ? Binding::Status::Satisfied
               : Binding::Status::Unsupported;
  }
  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage output) const
      -> Binding::Status {
    if (id == Ttx::Concept::Domain::contract_id) {
      static const Ttx::Concept::Domain::Operations operations = {
        [](const void* self, ttx_abstract* result) -> ttx_binding_status {
          *result = static_cast<const Widened*>(self)->type.get_abi();
          return TTX_BINDING_SATISFIED;
        }};
      const Ttx::Concept::Domain::Api api(this, &operations);
      return Binding::provide<Ttx::Concept::Domain>(api, output);
    }
    if (id == Execution::Value::contract_id) {
      const Execution::Value::Api api = {
        this, [](const void* self) -> ttx_semantic_query {
          return Abstract::provide(*static_cast<const Widened*>(self))
              .get_query();
        }};
      return Binding::provide<Execution::Value>(api, output);
    }
    if (id == Execution::Constant::contract_id) {
      return Binding::marker(output);
    }
    if (id == Ttx::Semantic::Transport::Block::Access::contract_id) {
      using Ttx::Semantic::Transport::Block;
      static const Block::Access::Operations operations = {
        [](const void*) -> const ttx_representation* {
          return &Type::Primitives::U32::get_representation();
        },
        [](const void* self, ttx_block_surface surface) -> ttx_data_status {
          const U32 value = static_cast<const Widened*>(self)->observation;
          Core::Data::copy(surface.data, &value);
          return TTX_DATA_SUCCESS;
        }};
      return Binding::provide<Block::Access>(
          Block::Access::Api(this, &operations), output);
    }
    return Binding::Status::Unsupported;
  }

 private:
  Abstract type;
  U8 observation;
};

// This policy explicitly chooses a conversion which the native U32 policy
// declines. Each request observes its source afresh and owns the returned
// snapshot, so retaining an earlier result cannot turn into a source cache.
class Widening {
 public:
  explicit Widening(Abstract target) : target(target) {}
  auto get_data() const -> Core::View::Bytes { return {}; }
  auto supports(System::Uuid id) const -> Binding::Status {
    return id == Type::Policies::Conversion::contract_id
               ? Binding::Status::Satisfied
               : target.supports(id);
  }
  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage output) const
      -> Binding::Status {
    if (id != Type::Policies::Conversion::contract_id) {
      return target.bind_interface(id, output);
    }
    const Type::Policies::Conversion::Api api = {
      this,
      [](const void* self, ttx_abstract value,
         ttx_abstract* result) -> ttx_binding_status {
        const auto& policy = *static_cast<const Widening*>(self);
        return Abstract(value).bind<Execution::Value>().visit(
            [&](Execution::Value value) -> ttx_binding_status {
              const auto& form = Type::Primitives::U8::get_representation();
              Ttx::Semantic::Transport::Flow flow;
              const auto status =
                  flow.connect(decltype(flow)::reader(form), value.get_value());
              if (status != decltype(flow)::Status::Success) {
                return status == decltype(flow)::Status::BindingPending
                           ? TTX_BINDING_PENDING
                           : TTX_BINDING_REJECTED;
              }

              U8 current = 0;
              const Ttx::Data::Form::Storage storage(
                  ttx_storage{&form, &current, sizeof(current)});
              if (Ttx::Semantic::Flows::Copy::flow(flow, storage) !=
                  Ttx::Data::Status::Success) {
                return TTX_BINDING_REJECTED;
              }

              const auto& projected =
                  policy.results.construct<Widened>(policy.target, current);
              *result = Abstract::provide(projected).get_abi();
              return TTX_BINDING_SATISFIED;
            },
            [](Binding::Failure failure) {
              return static_cast<ttx_binding_status>(failure);
            });
      }};
    return Binding::provide<Type::Policies::Conversion>(api, output);
  }

 private:
  Abstract target;
  mutable Memory::Allocator::Arena results;
};

PERIMORTEM_UNIT_TEST(Types, converted_observation) {
  const Type::Primitives::U8 byte_type;
  const Type::Primitives::U32 word_type;
  U8 current = 7;
  const auto& byte_form = byte_type.get_representation();
  const Execution::Assignments::Memory source(
      Abstract::provide(byte_type),
      Ttx::Data::Form::Storage(
          ttx_storage{&byte_form, &current, sizeof(current)}));
  const Widening target(Abstract::provide(word_type));

  // The generic primitive policy does not silently grant widening. The
  // explicit policy above supplies that different answer for the same source.
  Abstract::provide(word_type).bind<Type::Policies::Conversion>().visit(
      [&](Type::Policies::Conversion conversion) {
        conversion.convert(Abstract::provide(source))
            .visit(
                [&](Abstract) { EXPECT(False); },
                [&](Binding::Failure error) {
                  EXPECT(error == Binding::Failure::Rejected);
                });
      },
      [&](Binding::Failure) { EXPECT(False); });

  U32 stored = 0;
  const auto& word_form = word_type.get_representation();
  const Execution::Assignments::Memory destination(
      Abstract::provide(target),
      Ttx::Data::Form::Storage(
          ttx_storage{
            &word_form, reinterpret_cast<U8*>(&stored), sizeof(stored)}));
  destination.assign(Abstract::provide(source))
      .visit(
          [&](Ttx::Data::Status status) {
            EXPECT(status == Ttx::Data::Status::Success);
          },
          [&](Binding::Failure) { EXPECT(False); });
  EXPECT_EQ(stored, U32(7));

  current = 19;
  destination.assign(Abstract::provide(source))
      .visit(
          [&](Ttx::Data::Status status) {
            EXPECT(status == Ttx::Data::Status::Success);
          },
          [&](Binding::Failure) { EXPECT(False); });
  EXPECT_EQ(stored, U32(19));
}
