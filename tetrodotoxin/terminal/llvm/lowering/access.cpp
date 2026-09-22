// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/llvm/lowering/access.hpp"

#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/access/index.hpp"
#include "tetrodotoxin/library/language/access/propagate.hpp"
#include "tetrodotoxin/library/language/access/slice.hpp"
#include "tetrodotoxin/library/language/access/swizzle.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/access/unwrap.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/library/language/types/result.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/builtins.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/graph.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/types.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library::Language;

static auto lower_address(
    const Llvm::Lowering::Execution& execution,
    const Expression& expression,
    const Model::Pack& receiver) -> Bool {
  auto selected =
      expression.get_result().resolve().select<Tetrodotoxin::Source::Addressable>();
  auto instance =
      receiver.get_result().resolve().select<Tetrodotoxin::Source::Addressable>();
  BAIL_IF(
      !selected ||
      !Llvm::Lowering::Graph::prepare(execution.get_program(), *selected));
  if (!instance) {
    return execution.get_storage().select(expression, *selected);
  }
  return execution.lower(receiver) &&
         execution.get_storage().select_member(expression, *selected, receiver);
}

static auto lower_initializer(
    const Llvm::Lowering::Execution& execution,
    const Expressions::Initializer& initializer) -> Bool {
  auto type = initializer.get_type().resolve().select<Model::Type>();
  BAIL_IF(!type);

  if (initializer.uses_provider()) {
    auto structure = type->select<Types::Structure>();
    BAIL_IF(!structure || !execution.lower(initializer.get_arguments()));
    Memory::Dynamic::Vector<
        Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable>>
        parameters;
    for (const Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract>& candidate :
         structure->get_addressables()) {
      auto field = candidate.get().select<Field>();
      if (field && field->get_writability() == Writability::Internal &&
          field->get_definition().is_published()) {
        parameters.insert(*field);
      }
    }
    return execution.get_invocation().construct_provider(
        initializer, *type, initializer.get_arguments(), parameters.get_view());
  }

  auto values = initializer.get_completed_values();
  BAIL_IF(!values || !execution.lower(*values));
  auto completed_type = values->get_type().resolve().select<Tetrodotoxin::Source::Type>();
  if (completed_type && &*completed_type == &*type) {
    return execution.get_storage().alias(initializer, *values);
  }
  return execution.get_invocation().construct(initializer, *type, *values);
}

static auto lower_call(
    const Llvm::Lowering::Execution& execution,
    const Tetrodotoxin::Library::Language::Access::Call& call) -> Bool {
  auto callable = call.get_callable();
  BAIL_IF(
      !callable ||
      !Llvm::Lowering::Graph::prepare(execution.get_program(), *callable));
  if (callable->declares_self() && !execution.lower(call.get_receiver())) {
    return False;
  }
  BAIL_IF(!execution.lower(call.get_arguments()));

  Memory::Managed::Vector<LLVMValueRef> inputs(
      execution.get_program().get_arena());
  for (const Tetrodotoxin::Library::Language::Access::Call::Input& input :
       call.get_fitted_inputs()) {
    auto value = execution.get_invocation().fit_input(
        input.get_parameter(), input.get_source(), input.get_offset(),
        input.get_size());
    BAIL_IF(!value);
    inputs.insert(*value);
  }

  Core::Option<const Tetrodotoxin::Library::Language::Model::Pack&>
      receiver_source;
  if (callable->declares_self() && !call.get_fitted_inputs().is_empty()) {
    const Tetrodotoxin::Library::Language::Access::Call::Input& input =
        call.get_fitted_inputs().get_data()[0];
    if (input.get_offset() == 0 && input.get_size() == 1) {
      receiver_source = input.get_source();
    }
  }

  auto builtin = Llvm::Lowering::Builtins::lower(
      execution, *callable, call, inputs.get_view(), receiver_source);
  Core::Option<const Tetrodotoxin::Source::Pack&> generic_receiver =
      receiver_source.visit(
          []() -> Core::Option<const Tetrodotoxin::Source::Pack&> { return {}; },
          [](const Tetrodotoxin::Library::Language::Model::Pack& source)
              -> Core::Option<const Tetrodotoxin::Source::Pack&> { return source; });
  return builtin ? *builtin
                 : execution.get_invocation().invoke(
                       call, *callable, inputs.get_view(), generic_receiver);
}

static auto lower_slice(
    const Llvm::Lowering::Execution& execution,
    const Tetrodotoxin::Library::Language::Access::Slice& slice) -> Bool {
  auto element = slice.get_element_type();
  BAIL_IF(
      !element || !execution.lower(slice.get_receiver()) ||
      !execution.lower(slice.get_index()));
  auto count = slice.get_count();
  if (!count) {
    auto fallback = slice.get_fallback();
    auto state = execution.get_storage().begin_slice(
        *element, slice.get_receiver(), slice.get_index());
    return fallback && state && execution.lower(*fallback) &&
           execution.get_storage().end_slice(
               *state, *element, slice, *fallback);
  }

  auto range_count = slice.get_range_count();
  auto range = execution.get_storage().begin_slice_range(
      *element, slice.get_receiver(), slice.get_index());
  BAIL_IF(!range_count || !range);
  Memory::Managed::Vector<LLVMValueRef> values(
      execution.get_program().get_arena());
  for (Count offset = 0; offset < *range_count; offset++) {
    auto fallback =
        element->create_default(execution.get_program().get_arena());
    auto state = execution.get_storage().begin_slice_slot(*range, offset);
    BAIL_IF(!fallback || !state || !execution.lower(*fallback));
    auto selected =
        execution.get_storage().end_slice_slot(*state, *element, *fallback);
    BAIL_IF(!selected);
    values.insert(*selected);
  }
  return execution.get_storage().end_slice_range(slice, values.get_view());
}

static auto lower_propagation(
    const Llvm::Lowering::Execution& execution,
    const Tetrodotoxin::Library::Language::Access::Propagate& propagation)
    -> Bool {
  auto type =
      propagation.get_receiver().get_type().resolve().select<Model::Type>();
  BAIL_IF(!type || !execution.lower(propagation.get_receiver()));
  auto option = type->select<Types::Option>();
  if (option) {
    return execution.get_states().propagate_option(
        *option, option->get_element_type(), propagation,
        propagation.get_receiver(), propagation.get_escape());
  }
  auto flag = type->select<Model::Types::Flag>();
  if (flag) {
    return execution.get_states().propagate_flag(
        *flag, propagation, propagation.get_receiver(),
        propagation.get_escape());
  }
  auto result = type->select<Types::Result>();
  return result &&
         execution.get_states().propagate_result(
             *result, result->get_value_type(), result->get_error_type(),
             propagation, propagation.get_receiver(), propagation.get_escape());
}

auto Llvm::Lowering::Access::lower(
    const Execution& execution,
    const Expression& expression) -> Bool {
  auto identifier = expression.select<Expressions::Identifier>();
  if (identifier) {
    if (identifier->get_result().resolve().is<Model::Type>()) {
      return True;
    }
    auto addressable =
        identifier->get_result().resolve().select<Tetrodotoxin::Source::Addressable>();
    return addressable &&
           Graph::prepare(execution.get_program(), *addressable) &&
           execution.get_storage().select(*identifier, *addressable) &&
           execution.get_storage().load(*identifier);
  }
  auto address =
      expression.select<Tetrodotoxin::Library::Language::Access::Address>();
  if (address) {
    return lower_address(execution, *address, address->get_receiver()) &&
           execution.get_storage().load(*address);
  }
  auto initializer = expression.select<Expressions::Initializer>();
  if (initializer) {
    return lower_initializer(execution, *initializer);
  }
  auto call =
      expression.select<Tetrodotoxin::Library::Language::Access::Call>();
  if (call) {
    return lower_call(execution, *call);
  }
  auto slice =
      expression.select<Tetrodotoxin::Library::Language::Access::Slice>();
  if (slice) {
    return lower_slice(execution, *slice);
  }
  auto unwrap =
      expression.select<Tetrodotoxin::Library::Language::Access::Unwrap>();
  if (unwrap) {
    auto fallback = unwrap->get_fallback();
    auto carrier =
        unwrap->get_receiver().get_type().resolve().select<Tetrodotoxin::Source::Type>();
    auto element = unwrap->get_type().resolve().select<Tetrodotoxin::Source::Type>();
    BAIL_IF(
        !fallback || !carrier || !element ||
        !execution.lower(unwrap->get_receiver()));
    auto state = execution.get_states().begin_unwrap(
        *carrier, *element, unwrap->get_receiver());
    return state && execution.lower(*fallback) &&
           execution.get_states().end_unwrap(
               *state, *element, *unwrap, *fallback);
  }
  auto propagation =
      expression.select<Tetrodotoxin::Library::Language::Access::Propagate>();
  if (propagation) {
    return lower_propagation(execution, *propagation);
  }
  auto swizzle =
      expression.select<Tetrodotoxin::Library::Language::Access::Swizzle>();
  if (swizzle) {
    BAIL_IF(!execution.lower(swizzle->get_receiver()));
    Memory::Dynamic::Vector<LLVMValueRef> selected;
    for (const Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract>&
             projection : swizzle->get_projections()) {
      auto pack = Model::Pack::from(projection.get());
      BAIL_IF(!pack || !execution.lower(*pack));
      auto value = execution.get_body().find_value(*pack);
      BAIL_IF(!value);
      selected.insert(*value);
    }
    if (swizzle->get_projections().is_empty()) {
      auto receiver = execution.get_body().find_values(swizzle->get_receiver());
      BAIL_IF(!receiver);
      for (Count source : swizzle->get_selections()) {
        BAIL_IF(source >= receiver->get_size());
        selected.insert(receiver->get_data()[source]);
      }
    }
    return execution.get_body().publish_values(*swizzle, selected.get_view());
  }
  if (expression.is<Tetrodotoxin::Library::Language::Access::Type>()) {
    return True;
  }
  return False;
}

auto Llvm::Lowering::Access::lower_write_target(
    const Execution& execution,
    const Expression& expression) -> Bool {
  auto identifier = expression.select<Expressions::Identifier>();
  if (identifier) {
    auto addressable =
        identifier->get_result().resolve().select<Tetrodotoxin::Source::Addressable>();
    return addressable &&
           Graph::prepare(execution.get_program(), *addressable) &&
           execution.get_storage().select(*identifier, *addressable);
  }
  auto address =
      expression.select<Tetrodotoxin::Library::Language::Access::Address>();
  if (address) {
    return lower_address(execution, *address, address->get_receiver());
  }
  auto index =
      expression.select<Tetrodotoxin::Library::Language::Access::Index>();
  if (!index) {
    return False;
  }
  auto element = index->get_element_type().resolve().select<Tetrodotoxin::Source::Type>();
  BAIL_IF(
      !element || !execution.lower(index->get_receiver()) ||
      !execution.lower(index->get_index()));
  auto count = index->get_count();
  if (!count) {
    return execution.get_storage().select_index(
        *element, *index, index->get_receiver(), index->get_index());
  }
  auto range_count = index->get_range_count();
  return range_count && execution.lower(*count) &&
         execution.get_storage().select_range(
             *element, *index, index->get_receiver(), index->get_index(),
             *count, *range_count);
}
