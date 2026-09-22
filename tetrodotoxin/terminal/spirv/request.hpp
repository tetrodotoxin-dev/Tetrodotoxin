// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/shader/language/monograph.hpp"
#include "tetrodotoxin/shader/language/program.hpp"
#include "tetrodotoxin/terminal/spirv/target.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

namespace Tetrodotoxin::Terminal::Spirv {

// Request selects one completed Program from its Shader owner. Source facts
// stay borrowed only for actionable diagnostics while target policy remains a
// separate request fact.
class Request {
 public:
  constexpr Request(
      const Tetrodotoxin::Shader::Language::Monograph& monograph,
      const Tetrodotoxin::Shader::Language::Program& program,
      Tetrodotoxin::Source::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text,
      Target target)
      : monograph(monograph),
        program(program),
        errors(errors),
        source_path(source_path),
        source_text(source_text),
        target(target) {}

  constexpr auto get_monograph() const
      -> const Tetrodotoxin::Shader::Language::Monograph& {
    return monograph;
  }

  constexpr auto get_program() const
      -> const Tetrodotoxin::Shader::Language::Program& {
    return program;
  }

  constexpr auto get_errors() const -> Tetrodotoxin::Source::Lexical::Errors& { return errors; }
  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }
  constexpr auto get_source_text() const -> Perimortem::Core::View::Bytes {
    return source_text;
  }
  constexpr auto get_target() const -> Target { return target; }

 private:
  const Tetrodotoxin::Shader::Language::Monograph& monograph;
  const Tetrodotoxin::Shader::Language::Program& program;
  Tetrodotoxin::Source::Lexical::Errors& errors;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source_text;
  Target target;
};

}  // namespace Tetrodotoxin::Terminal::Spirv
