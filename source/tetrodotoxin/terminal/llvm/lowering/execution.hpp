// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/statement.hpp"
#include "tetrodotoxin/terminal/llvm/emission/computation.hpp"
#include "tetrodotoxin/terminal/llvm/emission/control_flow.hpp"
#include "tetrodotoxin/terminal/llvm/emission/invocation.hpp"
#include "tetrodotoxin/terminal/llvm/emission/states.hpp"
#include "tetrodotoxin/terminal/llvm/emission/storage.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Lowering {

// Execution walks one completed Library body through focused native owners.
// Keeping traversal here makes the handoff visible: Library exposes meaning
// and this Terminal decides how that meaning becomes target operations.
class Execution {
 public:
  constexpr explicit Execution(Module::Body& body)
      : body(body),
        computation(body),
        invocation(body),
        control_flow(body),
        states(body),
        storage(body) {}

  constexpr auto get_computation() const -> const Emission::Computation& {
    return computation;
  }

  constexpr auto get_invocation() const -> const Emission::Invocation& {
    return invocation;
  }

  constexpr auto get_control_flow() const -> const Emission::ControlFlow& {
    return control_flow;
  }

  constexpr auto get_states() const -> const Emission::States& {
    return states;
  }

  constexpr auto get_storage() const -> const Emission::Storage& {
    return storage;
  }

  constexpr auto get_program() const -> Module::Program& {
    return computation.get_program();
  }

  constexpr auto get_body() const -> Module::Body& { return body; }

  auto lower(const Tetrodotoxin::Library::Language::Model::Pack& pack) const
      -> Bool;

  auto lower(const Tetrodotoxin::Library::Language::Statement& statement) const
      -> Bool;

  auto lower(const Tetrodotoxin::Library::Language::Flow::Block& block) const
      -> Bool;

  auto lower_write_target(
      const Tetrodotoxin::Library::Language::Expression& expression) const
      -> Bool;

 private:
  Module::Body& body;
  Emission::Computation computation;
  Emission::Invocation invocation;
  Emission::ControlFlow control_flow;
  Emission::States states;
  Emission::Storage storage;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Lowering
