// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/llvm/lowering/operations.hpp"

#include "tetrodotoxin/library/language/expressions/conversion.hpp"
#include "tetrodotoxin/library/language/operation.hpp"
#include "tetrodotoxin/library/language/operations/add.hpp"
#include "tetrodotoxin/library/language/operations/add_assignment.hpp"
#include "tetrodotoxin/library/language/operations/and.hpp"
#include "tetrodotoxin/library/language/operations/assignment.hpp"
#include "tetrodotoxin/library/language/operations/divide.hpp"
#include "tetrodotoxin/library/language/operations/equal.hpp"
#include "tetrodotoxin/library/language/operations/greater.hpp"
#include "tetrodotoxin/library/language/operations/greater_equal.hpp"
#include "tetrodotoxin/library/language/operations/less.hpp"
#include "tetrodotoxin/library/language/operations/less_equal.hpp"
#include "tetrodotoxin/library/language/operations/modulo.hpp"
#include "tetrodotoxin/library/language/operations/multiply.hpp"
#include "tetrodotoxin/library/language/operations/negate.hpp"
#include "tetrodotoxin/library/language/operations/not.hpp"
#include "tetrodotoxin/library/language/operations/not_equal.hpp"
#include "tetrodotoxin/library/language/operations/or.hpp"
#include "tetrodotoxin/library/language/operations/range.hpp"
#include "tetrodotoxin/library/language/operations/subtract.hpp"
#include "tetrodotoxin/library/language/operations/subtract_assignment.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/types.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library::Language;
using Llvm::Emission::Computation;
using Llvm::Emission::States;
using Llvm::Emission::Storage;

static auto lower_inputs(
    const Llvm::Lowering::Execution& execution,
    const Operation& operation) -> Bool {
  for (const Tetrodotoxin::Source::PackReference<Model::Pack>& input :
       operation.get_inputs()) {
    if (!execution.lower(input.get())) {
      return False;
    }
  }
  return True;
}

static auto lower_arithmetic(
    const Llvm::Lowering::Execution& execution,
    const Operation& operation,
    Llvm::Emission::Computation::Arithmetic kind) -> Bool {
  auto inputs = operation.get_inputs();
  auto carrier = operation.get_type().resolve().select<Tetrodotoxin::Source::Type>();
  BAIL_IF(
      inputs.get_size() != 2 || !carrier ||
      !lower_inputs(execution, operation));
  return execution.get_computation().arithmetic(
      kind, *carrier, operation, inputs.get_data()[0].get(),
      inputs.get_data()[1].get());
}

static auto lower_comparison(
    const Llvm::Lowering::Execution& execution,
    const Operation& operation,
    Llvm::Emission::Computation::Comparison kind,
    Bool admits_bytes) -> Bool {
  auto inputs = operation.get_inputs();
  BAIL_IF(inputs.get_size() != 2);
  const Model::Pack& left = inputs.get_data()[0].get();
  auto carrier = left.get_type().resolve().select<Tetrodotoxin::Source::Type>();
  BAIL_IF(!carrier || !lower_inputs(execution, operation));
  if (admits_bytes && carrier->is<Types::View>()) {
    return execution.get_computation().compare_bytes(
        kind, operation, left, inputs.get_data()[1].get());
  }
  return execution.get_computation().compare(
      kind, *carrier, operation, left, inputs.get_data()[1].get());
}

auto Llvm::Lowering::Operations::lower(
    const Execution& execution,
    const Expression& expression) -> Bool {
  auto conversion =
      expression
          .select<Tetrodotoxin::Library::Language::Expressions::Conversion>();
  if (conversion) {
    auto source_type =
        conversion->get_source()
            .get_value_type(0)
            .resolve()
            .select<Tetrodotoxin::Library::Language::Model::Type>();
    return source_type &&
           Types::prepare(execution.get_program(), *source_type) &&
           Types::prepare(execution.get_program(), conversion->get_type()) &&
           execution.lower(conversion->get_source()) &&
           execution.get_computation().convert(
               *source_type, conversion->get_type(), *conversion,
               conversion->get_source());
  }

  auto add =
      expression.select<Tetrodotoxin::Library::Language::Operations::Add>();
  if (add) {
    return lower_arithmetic(execution, *add, Computation::Arithmetic::Add);
  }
  auto subtract =
      expression
          .select<Tetrodotoxin::Library::Language::Operations::Subtract>();
  if (subtract) {
    return lower_arithmetic(
        execution, *subtract, Computation::Arithmetic::Subtract);
  }
  auto multiply =
      expression
          .select<Tetrodotoxin::Library::Language::Operations::Multiply>();
  if (multiply) {
    return lower_arithmetic(
        execution, *multiply, Computation::Arithmetic::Multiply);
  }
  auto divide =
      expression.select<Tetrodotoxin::Library::Language::Operations::Divide>();
  if (divide) {
    return lower_arithmetic(
        execution, *divide, Computation::Arithmetic::Divide);
  }
  auto modulo =
      expression.select<Tetrodotoxin::Library::Language::Operations::Modulo>();
  if (modulo) {
    return lower_arithmetic(
        execution, *modulo, Computation::Arithmetic::Modulo);
  }

  auto negate =
      expression.select<Tetrodotoxin::Library::Language::Operations::Negate>();
  if (negate) {
    auto inputs = negate->get_inputs();
    auto carrier = negate->get_type().resolve().select<Tetrodotoxin::Source::Type>();
    return inputs.get_size() == 1 && carrier &&
           execution.lower(inputs.get_data()[0].get()) &&
           execution.get_computation().negate(
               *carrier, *negate, inputs.get_data()[0].get());
  }

  auto equal =
      expression.select<Tetrodotoxin::Library::Language::Operations::Equal>();
  if (equal) {
    return lower_comparison(
        execution, *equal, Computation::Comparison::Equal, True);
  }
  auto not_equal =
      expression
          .select<Tetrodotoxin::Library::Language::Operations::NotEqual>();
  if (not_equal) {
    return lower_comparison(
        execution, *not_equal, Computation::Comparison::NotEqual, True);
  }
  auto less =
      expression.select<Tetrodotoxin::Library::Language::Operations::Less>();
  if (less) {
    return lower_comparison(
        execution, *less, Computation::Comparison::Less, False);
  }
  auto less_equal =
      expression
          .select<Tetrodotoxin::Library::Language::Operations::LessEqual>();
  if (less_equal) {
    return lower_comparison(
        execution, *less_equal, Computation::Comparison::LessEqual, False);
  }
  auto greater =
      expression.select<Tetrodotoxin::Library::Language::Operations::Greater>();
  if (greater) {
    return lower_comparison(
        execution, *greater, Computation::Comparison::Greater, False);
  }
  auto greater_equal =
      expression
          .select<Tetrodotoxin::Library::Language::Operations::GreaterEqual>();
  if (greater_equal) {
    return lower_comparison(
        execution, *greater_equal, Computation::Comparison::GreaterEqual,
        False);
  }

  auto logical_not =
      expression.select<Tetrodotoxin::Library::Language::Operations::Not>();
  if (logical_not) {
    auto inputs = logical_not->get_inputs();
    return inputs.get_size() == 1 &&
           execution.lower(inputs.get_data()[0].get()) &&
           execution.get_states().logical_not(
               *logical_not, inputs.get_data()[0].get());
  }

  auto logical_and =
      expression.select<Tetrodotoxin::Library::Language::Operations::And>();
  auto logical_or =
      expression.select<Tetrodotoxin::Library::Language::Operations::Or>();
  if (logical_and || logical_or) {
    const Operation& operation =
        logical_and ? static_cast<const Operation&>(*logical_and)
                    : static_cast<const Operation&>(*logical_or);
    auto inputs = operation.get_inputs();
    BAIL_IF(
        inputs.get_size() != 2 || !execution.lower(inputs.get_data()[0].get()));
    auto state = execution.get_states().begin_logic(
        logical_and ? States::Logical::And : States::Logical::Or,
        inputs.get_data()[0].get());
    return state && execution.lower(inputs.get_data()[1].get()) &&
           execution.get_states().end_logic(
               *state, operation, inputs.get_data()[0].get(),
               inputs.get_data()[1].get());
  }

  auto range =
      expression.select<Tetrodotoxin::Library::Language::Operations::Range>();
  if (range) {
    auto inputs = range->get_inputs();
    auto carrier = range->get_type().resolve().select<Model::Type>();
    return inputs.get_size() == 2 && carrier &&
           Types::prepare(execution.get_program(), *carrier) &&
           lower_inputs(execution, *range) &&
           execution.get_storage().range(
               *carrier, *range, inputs.get_data()[0].get(),
               inputs.get_data()[1].get());
  }

  auto assignment =
      expression
          .select<Tetrodotoxin::Library::Language::Operations::Assignment>();
  if (assignment) {
    return execution.lower_write_target(assignment->get_target()) &&
           execution.lower(assignment->get_source()) &&
           execution.get_storage().write(
               Storage::Write::Assign, *assignment, assignment->get_target(),
               assignment->get_source());
  }
  auto add_assignment =
      expression
          .select<Tetrodotoxin::Library::Language::Operations::AddAssignment>();
  if (add_assignment) {
    return execution.lower_write_target(add_assignment->get_target()) &&
           execution.lower(add_assignment->get_right()) &&
           execution.get_storage().write(
               Storage::Write::Add, *add_assignment,
               add_assignment->get_target(), add_assignment->get_right());
  }
  auto subtract_assignment = expression.select<
      Tetrodotoxin::Library::Language::Operations::SubtractAssignment>();
  if (subtract_assignment) {
    return execution.lower_write_target(subtract_assignment->get_target()) &&
           execution.lower(subtract_assignment->get_right()) &&
           execution.get_storage().write(
               Storage::Write::Subtract, *subtract_assignment,
               subtract_assignment->get_target(),
               subtract_assignment->get_right());
  }

  return False;
}
