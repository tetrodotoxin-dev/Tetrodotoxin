// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/model/layout.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/layout.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Signature owns exactly the authored parameter and result Layout models for
// one Function. Each model retains its source descriptors and final semantic
// entries, so Signature coordinates the two roles without copying slots,
// names, Type routes, or resolved Layouts into another representation.
class Signature {
 public:
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Abstract& host,
      Model::Layout& parameters,
      Model::Layout& results) -> Signature&;

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Abstract& host,
      Model::Layout& parameters,
      Model::Layout& results) -> Signature&;

  auto link_restored() -> Bool;

  Signature(const Signature&) = delete;
  Signature(Signature&&) = delete;
  auto operator=(const Signature&) -> Signature& = delete;
  auto operator=(Signature&&) -> Signature& = delete;

  auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

  constexpr auto get_parameters() const -> const Model::Layout& {
    return parameters;
  }
  constexpr auto get_results() const -> const Model::Layout& { return results; }

  constexpr auto edit_parameters() -> Model::Layout& { return parameters; }
  constexpr auto edit_results() -> Model::Layout& { return results; }

  auto declares_self() const -> Bool;
  auto is_linked() const -> Bool;

  // Signature owns publication validation for its two exact Layouts. Function
  // invokes this semantic operation without receiving private access to the
  // Signature representation.
  auto validate_publication(Tetrodotoxin::Source::Lexical::Cursor& cursor) const -> Bool;

 private:
  constexpr Signature(
      const Tetrodotoxin::Source::Abstract& host,
      Model::Layout& parameters,
      Model::Layout& results)
      : host(host), parameters(parameters), results(results) {}

  const Tetrodotoxin::Source::Abstract& host;
  Model::Layout& parameters;
  Model::Layout& results;
};

}  // namespace Tetrodotoxin::Library::Language
