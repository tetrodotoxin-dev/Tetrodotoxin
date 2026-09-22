// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/llvm/lowering/execution.hpp"

#include "tetrodotoxin/terminal/llvm/lowering/access.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/control.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/operations.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/values.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;

auto Llvm::Lowering::Execution::lower(
    const Tetrodotoxin::Library::Language::Model::Pack& pack) const -> Bool {
  // Constants supply their own Pack after separation from authored Expressions.
  // Sending them through composition would silently turn a scalar into no
  // values.
  auto constant =
      pack.select_identity<Tetrodotoxin::Library::Language::Constant>();
  if (constant) {
    return Values::lower(*this, *constant);
  }
  auto expression =
      pack.select_identity<Tetrodotoxin::Library::Language::Expression>();
  if (expression) {
    auto folded = expression->get_folded();
    if (folded && &*folded != &pack) {
      return lower(*folded) && storage.alias(pack, *folded);
    }

    if (Operations::lower(*this, *expression)) {
      return True;
    }
    return Access::lower(*this, *expression);
  }

  for (const Tetrodotoxin::Source::PackReference<
           Tetrodotoxin::Library::Language::Model::Pack>& entry :
       pack.get_entries()) {
    if (!lower(entry.get())) {
      return False;
    }
  }
  return storage.compose(pack);
}

auto Llvm::Lowering::Execution::lower(
    const Tetrodotoxin::Library::Language::Statement& statement) const -> Bool {
  return Control::lower(*this, statement);
}

auto Llvm::Lowering::Execution::lower(
    const Tetrodotoxin::Library::Language::Flow::Block& block) const -> Bool {
  return Control::lower(*this, block);
}

auto Llvm::Lowering::Execution::lower_write_target(
    const Tetrodotoxin::Library::Language::Expression& expression) const
    -> Bool {
  return Access::lower_write_target(*this, expression);
}
