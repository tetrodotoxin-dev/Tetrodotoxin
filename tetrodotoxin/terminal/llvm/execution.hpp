// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/semantic/realization/invocation.hpp"

namespace Tetrodotoxin::Terminal::Llvm {

// Execution compiles a negotiated model into an independently owned object
// file. It retains object code and realized frame descriptions, so the complete
// source graph may be released after compilation. The supplying dialect and
// source locations have no role in runtime dispatch.
//
// This terminal lowers parameter uses, constants and Return through each
// Type's selected Storage form. Scalars and plain records share that path.
// Other bodies decline through their negotiated capabilities. Adding another
// body requires its lowering here, independently of the dialect that supplies
// it. Invocation uses the generic TTX frame contract. Each object exports
// ttx_entry(input_frame, output_frame) with the Linux x86_64 C convention. The
// linker owns symbol placement and executable lifetime. The two frame
// descriptions supply its complete input and output geometry.
class Execution {
 public:
  static auto compile(Ttx::Concept::Abstract subject) -> Perimortem::Utility::
      Result<Execution, Ttx::Semantic::Negotiation::Binding::Failure>;
  Execution(Execution&& other) = default;
  Execution(const Execution&) = delete;
  auto operator=(const Execution&) -> Execution& = delete;

  auto get_object() const -> Perimortem::Core::View::Bytes {
    return object.get_view();
  }

  auto get_inputs() const -> const Ttx::Data::Form::Representation& {
    return *inputs;
  }

  auto get_outputs() const -> const Ttx::Data::Form::Representation& {
    return *outputs;
  }

  auto get_operation() const -> Perimortem::System::Uuid { return operation; }

 private:
  Execution() = default;

  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Dynamic::Bytes object;
  Perimortem::System::Uuid operation;
  const Ttx::Data::Form::Representation* inputs = nullptr;
  const Ttx::Data::Form::Representation* outputs = nullptr;
};

}  // namespace Tetrodotoxin::Terminal::Llvm
