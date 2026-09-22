// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/llvm/lowering/control.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/library/language/flow/branch.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/flow/loop_control.hpp"
#include "tetrodotoxin/library/language/flow/match.hpp"
#include "tetrodotoxin/library/language/flow/range_loop.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/types/contiguous.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/scene/language/emission.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/scene.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/types.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library::Language;
using Llvm::Emission::ControlFlow;

static auto lower_local(
    const Llvm::Lowering::Execution& execution,
    const Flow::Local& local) -> Bool {
  const ControlFlow& body = execution.get_control_flow();
  if (local.get_writability() == Writability::Constant) {
    if (!body.has_full_debug()) {
      return True;
    }
    auto value = local.get_constant();
    return value && execution.lower(*value) &&
           body.constant_local(local, *value, local.get_anchor());
  }

  auto type = local.get_type().select<Model::Type>();
  BAIL_IF(
      !type || !Llvm::Lowering::Types::prepare(execution.get_program(), *type));
  Core::Option<const Model::Pack&> value = local.get_initializer();
  if (!value) {
    auto created = type->create_default(execution.get_program().get_arena());
    BAIL_IF(!created);
    value = *created;
  }
  return execution.lower(*value) && body.bind_local(local, *value) &&
         body.local(local, local.get_anchor());
}

static auto lower_branch(
    const Llvm::Lowering::Execution& execution,
    const Flow::Branch& branch) -> Bool {
  const ControlFlow& body = execution.get_control_flow();
  if (branch.get_kind() == Flow::Branch::Kind::While) {
    return body.begin_while(branch) &&
           execution.lower(branch.get_condition()) &&
           body.select_while(branch, branch.get_condition()) &&
           execution.lower(branch.get_body()) && body.end_while(branch);
  }
  BAIL_IF(!execution.lower(branch.get_condition()));
  auto state = body.begin_branch(branch.get_condition());
  BAIL_IF(!state || !execution.lower(branch.get_body()));
  auto alternate = branch.get_alternate();
  if (alternate &&
      (!body.begin_alternate(*state) || !execution.lower(*alternate))) {
    return False;
  }
  return body.end_branch(*state);
}

static auto begin_iteration(
    const Llvm::Lowering::Execution& execution,
    const Flow::RangeLoop& loop,
    const Model::Type& type) -> Bool {
  const ControlFlow& body = execution.get_control_flow();
  const Tetrodotoxin::Source::Layout& bindings = loop.get_bindings();
  if (type.is<Types::Range>() || type.is<Types::Contiguous>()) {
    auto entry = bindings.get_abstract(0);
    auto binding = entry ? entry->select<Tetrodotoxin::Source::Addressable>()
                         : Core::Option<const Tetrodotoxin::Source::Addressable&>();
    return binding &&
           body.begin_sequence(loop, *binding, type, loop.get_input());
  }
  auto enumeration = type.select<Types::Enumeration>();
  BAIL_IF(!enumeration);
  Memory::Dynamic::Vector<U64> values;
  Memory::Dynamic::Vector<Core::View::Bytes> names;
  values.resize(enumeration->get_cases().get_size());
  if (bindings.get_size() == 2) {
    names.resize(enumeration->get_cases().get_size());
  }
  for (Count index = 0; index < enumeration->get_cases().get_size(); index++) {
    auto value = enumeration->get_case_value(index);
    BAIL_IF(!value);
    values[index] = *value;
    if (names.get_size()) {
      names[index] = enumeration->get_case_name(index);
    }
  }
  return body.begin_enumeration(
      loop, bindings, values.get_view(), names.get_view());
}

static auto lower_loop(
    const Llvm::Lowering::Execution& execution,
    const Flow::RangeLoop& loop) -> Bool {
  auto type = loop.get_input_type();
  if (!type) {
    Perimortem::Core::Diagnostics::Log::error(
        "LLVM received a Library loop without its completed input Type."_view);
    return False;
  }
  if (!execution.lower(loop.get_input())) {
    Perimortem::Core::Diagnostics::Log::error(
        "LLVM could not emit the input for a Library loop."_view);
    return False;
  }
  if (!begin_iteration(execution, loop, *type)) {
    Perimortem::Core::Diagnostics::Log::error(
        "LLVM could not begin iteration for the selected Library Type."_view);
    return False;
  }
  if (!execution.lower(loop.get_body())) {
    Perimortem::Core::Diagnostics::Log::error(
        "LLVM could not emit a Library loop body."_view);
    return False;
  }
  return execution.get_control_flow().end_iteration(loop);
}

static auto lower_match(
    const Llvm::Lowering::Execution& execution,
    const Flow::Match& match) -> Bool {
  const ControlFlow& body = execution.get_control_flow();
  BAIL_IF(!execution.lower(match.get_input()));
  auto state = body.begin_match(match.get_input());
  BAIL_IF(!state);
  for (Count index = 0; index < match.get_case_count(); index++) {
    auto kind = match.get_case_kind(index);
    auto case_body = match.get_case_body(index);
    BAIL_IF(!kind || !case_body);
    Core::Option<ControlFlow::MatchCase> selected;
    if (*kind == Flow::Match::CaseKind::Constant) {
      auto constant = match.get_case_constant(index);
      BAIL_IF(!constant || !execution.lower(*constant));
      selected = body.begin_constant_case(*state, *constant);
    } else {
      auto payload = match.get_case_payload(index);
      auto anchor = match.get_case_anchor(index);
      BAIL_IF(!payload || !anchor);
      selected = body.begin_value_case(*state, *payload, *anchor);
    }
    BAIL_IF(
        !selected || !execution.lower(*case_body) ||
        !body.end_match_case(*state, *selected));
  }
  auto fallback = match.get_default();
  if (fallback) {
    auto selected = body.begin_default_case();
    BAIL_IF(
        !execution.lower(*fallback) || !body.end_match_case(*state, selected));
  }
  return body.end_match(*state, !match.has_complete_coverage());
}

static auto lower_root(
    const Llvm::Lowering::Execution& execution,
    const Tetrodotoxin::Source::Abstract& root) -> Bool {
  auto local = root.select<Flow::Local>();
  if (local) {
    return lower_local(execution, *local);
  }
  auto returned = root.select<Flow::Return>();
  if (returned) {
    return execution.lower(returned->get_pack()) &&
           execution.get_control_flow().return_values(returned->get_pack());
  }
  auto loop_control = root.select<Flow::LoopControl>();
  if (loop_control) {
    auto action = loop_control->get_kind() == Flow::LoopControl::Kind::Break
                      ? ControlFlow::LoopAction::Break
                      : ControlFlow::LoopAction::Continue;
    return execution.get_control_flow().leave_loop(
        action, loop_control->get_target());
  }
  auto branch = root.select<Flow::Branch>();
  if (branch) {
    return lower_branch(execution, *branch);
  }
  auto loop = root.select<Flow::RangeLoop>();
  if (loop) {
    return lower_loop(execution, *loop);
  }
  auto match = root.select<Flow::Match>();
  if (match) {
    return lower_match(execution, *match);
  }
  auto emission = root.select<Tetrodotoxin::Scene::Language::Emission>();
  if (emission) {
    return Llvm::Lowering::Scene::lower(execution, *emission);
  }
  auto block = root.select<Flow::Block>();
  return block && execution.lower(*block);
}

auto Llvm::Lowering::Control::lower(
    const Execution& execution,
    const Statement& statement) -> Bool {
  const ControlFlow& body = execution.get_control_flow();
  if (!body.statement(statement.get_anchor())) {
    return False;
  }
  auto pack = statement.get_pack();
  if (!(pack ? execution.lower(*pack)
             : lower_root(execution, statement.get_root()))) {
    Perimortem::Core::Diagnostics::Log::Message<256> message(
        Perimortem::Core::Diagnostics::Log::Level::Error,
        Perimortem::Core::Diagnostics::Source());
    message << "LLVM could not emit Library Statement root '"_view
            << statement.get_root().get_name() << "'."_view;
    return False;
  }
  return body.end_statement();
}

auto Llvm::Lowering::Control::lower(
    const Execution& execution,
    const Flow::Block& block) -> Bool {
  const ControlFlow& body = execution.get_control_flow();
  BAIL_IF(!body.begin_block(block, block.get_anchor()));
  for (const Statement& statement : block.get_statements()) {
    BAIL_IF(!execution.lower(statement));
  }
  return body.end_block(block);
}
