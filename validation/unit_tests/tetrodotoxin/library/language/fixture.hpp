// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/span.hpp"
#include "tetrodotoxin/source/lexical/tokenizer.hpp"

namespace Validation {

// Direct semantic unit tests still use a real Library root. The temporary
// Cursor only opens that root in the supplied Arena. Every resulting identity
// remains owned by the Arena and is reached through the Monograph graph.
inline auto create_library_monograph(
    Perimortem::Memory::Allocator::Arena& arena,
    Tetrodotoxin::Library::Dialect& dialect)
    -> Tetrodotoxin::Library::Language::Monograph& {
  Tetrodotoxin::Source::Lexical::Errors errors;
  Tetrodotoxin::Source::Lexical::Tokenizer tokenizer(arena, {}, {});
  Tetrodotoxin::Source::Lexical::Associations associations(tokenizer.get_arena());
  Tetrodotoxin::Source::Lexical::Cursor cursor(tokenizer, errors, associations);
  return Tetrodotoxin::Library::Language::Monograph::create_authored(
      cursor.get_arena(), Tetrodotoxin::Source::Documentation::get_empty(),
      Tetrodotoxin::Source::Lexical::Anchor::create(Tetrodotoxin::Source::Lexical::Span()), dialect, dialect);
}

inline auto resolve_library_flag(
    const Tetrodotoxin::Library::Language::Monograph& monograph)
    -> const Tetrodotoxin::Library::Language::Model::Types::Flag& {
  return static_cast<
      const Tetrodotoxin::Library::Language::Model::Types::Flag&>(
      monograph.resolve_concept("Bool"_view));
}

inline auto resolve_library_unsigned(
    const Tetrodotoxin::Library::Language::Monograph& monograph,
    Perimortem::Core::View::Bytes name)
    -> const Tetrodotoxin::Library::Language::Model::Types::Unsigned& {
  return static_cast<
      const Tetrodotoxin::Library::Language::Model::Types::Unsigned&>(
      monograph.resolve_concept(name));
}

inline auto resolve_library_signed(
    const Tetrodotoxin::Library::Language::Monograph& monograph,
    Perimortem::Core::View::Bytes name)
    -> const Tetrodotoxin::Library::Language::Model::Types::Signed& {
  return static_cast<
      const Tetrodotoxin::Library::Language::Model::Types::Signed&>(
      monograph.resolve_concept(name));
}

inline auto resolve_library_real(
    const Tetrodotoxin::Library::Language::Monograph& monograph,
    Perimortem::Core::View::Bytes name)
    -> const Tetrodotoxin::Library::Language::Model::Types::Real& {
  return static_cast<
      const Tetrodotoxin::Library::Language::Model::Types::Real&>(
      monograph.resolve_concept(name));
}

inline auto resolve_library_type(
    const Tetrodotoxin::Library::Language::Monograph& monograph,
    Perimortem::Core::View::Bytes name)
    -> const Tetrodotoxin::Library::Language::Model::Type& {
  return static_cast<const Tetrodotoxin::Library::Language::Model::Type&>(
      monograph.resolve_concept(name));
}

}  // namespace Validation
