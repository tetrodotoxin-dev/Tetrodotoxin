// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/result.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/linker/import.hpp"
#include "tetrodotoxin/terminal/abi/export.hpp"
#include "tetrodotoxin/terminal/abi/products.hpp"
#include "tetrodotoxin/terminal/abi/publication.hpp"
#include "tetrodotoxin/terminal/abi/unit.hpp"
#include "tetrodotoxin/terminal/llvm/failure.hpp"
#include "tetrodotoxin/terminal/llvm/module/carriers.hpp"
#include "tetrodotoxin/terminal/llvm/module/debug.hpp"
#include "tetrodotoxin/terminal/llvm/module/emission.hpp"
#include "tetrodotoxin/terminal/llvm/module/functions.hpp"
#include "tetrodotoxin/terminal/llvm/module/globals.hpp"
#include "tetrodotoxin/terminal/llvm/products.hpp"
#include "tetrodotoxin/terminal/llvm/target.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Module {

// Program owns one LLVM module transaction and publishes Terminal bytes only
// after LLVM traversal and verification complete.
class Program : public Emission {
 public:
  Program(
      Perimortem::Memory::Allocator::Arena& arena,
      Tetrodotoxin::Source::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text,
      Target target,
      Debug::Level debug_level,
      Tetrodotoxin::Terminal::Abi::Unit unit,
      const Tetrodotoxin::Terminal::Abi::Products& native_interface);

  ~Program();

  auto initialize() -> Bool;

  // Creates one hosted process entry that invokes an already published native
  // symbol with empty parameter and result shapes exactly once and then returns
  // success to the host.
  auto create_process_entry(Perimortem::Core::View::Bytes symbol) -> Bool;

  auto compile() -> Perimortem::Utility::Result<Products, Failure>;

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }

  constexpr auto get_target() const -> Target { return target; }

  constexpr auto get_debug() -> Debug& { return debug; }

  constexpr auto get_debug() const -> const Debug& { return debug; }

  constexpr auto get_context() -> LLVMOpaqueContext& { return context; }

  constexpr auto get_module() -> LLVMOpaqueModule& { return module; }

  constexpr auto get_carriers() const -> const Carriers& { return carriers; }

  constexpr auto get_functions() const -> const Functions& { return functions; }

  constexpr auto get_globals() const -> const Globals& { return globals; }

  constexpr auto get_unit() const -> const Tetrodotoxin::Terminal::Abi::Unit& {
    return unit;
  }

  constexpr auto get_interface() const
      -> const Tetrodotoxin::Terminal::Abi::Products& {
    return native_interface;
  }

  auto add_export(Tetrodotoxin::Terminal::Abi::Export value) -> void;

  auto add_publication(Tetrodotoxin::Terminal::Abi::Publication value) -> void;

  auto add_import(Tetrodotoxin::Linker::Import value) -> Bool;

  auto fail_toolchain(Perimortem::Core::View::Bytes message) -> Bool;

  auto fail_source(
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> Bool;

  constexpr auto has_source_failure() const -> Bool {
    return Bool(failures & U8(FailureFlag::Source));
  }

  constexpr auto has_tool_failure() const -> Bool {
    return Bool(failures & U8(FailureFlag::Tool));
  }

 private:
  enum class FailureFlag : U8 {
    Source = 1,
    Tool = 2,
  };

  Perimortem::Memory::Allocator::Arena& arena;
  Tetrodotoxin::Source::Lexical::Errors& errors;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source_text;
  Target target;
  Tetrodotoxin::Terminal::Abi::Unit unit;
  const Tetrodotoxin::Terminal::Abi::Products& native_interface;
  Debug debug;
  LLVMOpaqueContext& context;
  LLVMOpaqueModule& module;
  Perimortem::Core::Option<U8&> target_machine;
  Carriers carriers;
  Functions functions;
  Globals globals;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Linker::Import> imports;
  U8 failures = 0;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Module
