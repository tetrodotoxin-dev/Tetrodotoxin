// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Option is one completed immutable optional value. A present value retains
// its complete folded payload Pack without copying any producer identity.
class Option : public Tetrodotoxin::Library::Language::Constant {
 public:
  TTX_CONTRACT(Option, Tetrodotoxin::Library::Language::Constant);

  static auto create_absent(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Option& type) -> Option&;

  static auto create_present(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Option& type,
      Model::Pack& payload) -> Perimortem::Core::Option<Option&>;

  static auto create_fitted(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Option& type,
      Model::Pack& source) -> Perimortem::Core::Option<Option&>;

  constexpr auto get_type() const -> const Types::Option& override {
    return type;
  }

  constexpr auto get_kind() const -> Types::Option::Kind { return kind; }

  auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name.get_view();
  }

  auto get_payload() const -> Perimortem::Core::Option<const Model::Pack&>;

  auto equals(const Tetrodotoxin::Library::Language::Constant& rhs) const
      -> Bool override;

 private:
  Option(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Option& type,
      Types::Option::Kind kind,
      Perimortem::Core::Option<Tetrodotoxin::Source::PackReference<Model::Pack>> payload,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor);

  const Types::Option& type;
  Types::Option::Kind kind;
  Perimortem::Core::Option<Tetrodotoxin::Source::PackReference<Model::Pack>> payload;
  Perimortem::Memory::Managed::Bytes name;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
