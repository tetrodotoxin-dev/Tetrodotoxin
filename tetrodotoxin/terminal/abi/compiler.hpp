// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/terminal/abi/products.hpp"
#include "tetrodotoxin/terminal/abi/unit.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

namespace Tetrodotoxin::Terminal::Abi {

// Compiler projects one completed Library graph into native interface
// products. It chooses no instruction producer and leaves every semantic owner
// in the source Monograph.
class Compiler {
 public:
  auto compile_graph(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Library::Language::Monograph& monograph,
      const Tetrodotoxin::Source::Abstract& root,
      const Abi::Unit& unit,
      Tetrodotoxin::Source::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text) const
      -> Perimortem::Core::Option<Products>;

  auto compile(
      Perimortem::Memory::Allocator::Arena& arena,
      const Tetrodotoxin::Library::Language::Monograph& monograph,
      const Abi::Unit& unit,
      Tetrodotoxin::Source::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<
          const Tetrodotoxin::Library::Language::Model::Type>> roots = {},
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<
          const Tetrodotoxin::Library::Language::Model::Callable>> excluded =
          {},
      Perimortem::Core::View::Vector<Abi::Projection> projections = {}) const
      -> Perimortem::Core::Option<Products>;
};

}  // namespace Tetrodotoxin::Terminal::Abi
