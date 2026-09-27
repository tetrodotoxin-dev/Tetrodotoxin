// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/terminal/abi/products.hpp"
#include "tetrodotoxin/terminal/abi/unit.hpp"
#include "tetrodotoxin/terminal/graphics/products.hpp"
#include "tetrodotoxin/terminal/llvm/module/debug.hpp"
#include "tetrodotoxin/terminal/llvm/target.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

namespace Tetrodotoxin::Terminal::Llvm {

// Request borrows one completed Library graph and the exact authored source
// facts used by this compilation. Target and Debug Level are request policy
// rather than semantic graph facts.
class Request {
 public:
  constexpr Request(
      const Tetrodotoxin::Library::Language::Monograph& monograph,
      Tetrodotoxin::Source::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text,
      Target target,
      Module::Debug::Level debug_level,
      Tetrodotoxin::Terminal::Abi::Unit unit,
      const Tetrodotoxin::Terminal::Abi::Products& native_interface,
      Perimortem::Core::Option<
          const Tetrodotoxin::Terminal::Graphics::Products&> graphics = {},
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<
          const Tetrodotoxin::Library::Language::Model::Callable>> excluded =
          {})
      : monograph(monograph),
        errors(errors),
        source_path(source_path),
        source_text(source_text),
        target(target),
        debug_level(debug_level),
        unit(unit.bind(monograph)),
        native_interface(native_interface),
        graphics(graphics),
        excluded(excluded) {}

  constexpr auto get_monograph() const
      -> const Tetrodotoxin::Library::Language::Monograph& {
    return monograph;
  }

  constexpr auto get_errors() const -> Tetrodotoxin::Source::Lexical::Errors& { return errors; }

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }

  constexpr auto get_source_text() const -> Perimortem::Core::View::Bytes {
    return source_text;
  }

  constexpr auto get_target() const -> Target { return target; }
  constexpr auto get_debug_level() const -> Module::Debug::Level {
    return debug_level;
  }

  constexpr auto get_unit() const -> const Tetrodotoxin::Terminal::Abi::Unit& {
    return unit;
  }

  constexpr auto get_interface() const
      -> const Tetrodotoxin::Terminal::Abi::Products& {
    return native_interface;
  }

  constexpr auto get_graphics() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Terminal::Graphics::Products&> {
    return graphics;
  }

  constexpr auto get_excluded() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<
          const Tetrodotoxin::Library::Language::Model::Callable>> {
    return excluded;
  }

 private:
  const Tetrodotoxin::Library::Language::Monograph& monograph;
  Tetrodotoxin::Source::Lexical::Errors& errors;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source_text;
  Target target;
  Module::Debug::Level debug_level;
  Tetrodotoxin::Terminal::Abi::Unit unit;
  const Tetrodotoxin::Terminal::Abi::Products& native_interface;
  Perimortem::Core::Option<const Tetrodotoxin::Terminal::Graphics::Products&>
      graphics;
  Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<
      const Tetrodotoxin::Library::Language::Model::Callable>>
      excluded;
};

}  // namespace Tetrodotoxin::Terminal::Llvm
