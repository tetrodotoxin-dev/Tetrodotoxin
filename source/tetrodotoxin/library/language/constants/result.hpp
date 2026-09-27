// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/types/result.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Result is one completed immutable value or error selection. Its payload Pack
// retains the exact folded alternative without copying producer identity.
class Result : public Tetrodotoxin::Library::Language::Constant {
 public:
  TTX_CONTRACT(Result, Tetrodotoxin::Library::Language::Constant);

  static auto create_value(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Result& type,
      Model::Pack& payload) -> Perimortem::Core::Option<Result&>;

  static auto create_error(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Result& type,
      Model::Pack& payload) -> Perimortem::Core::Option<Result&>;

  static auto create_fitted(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Result& type,
      Model::Pack& source) -> Perimortem::Core::Option<Result&>;

  static auto select(Model::Pack& source) -> Perimortem::Core::Option<Result&>;

  constexpr auto get_type() const -> const Types::Result& override {
    return type;
  }

  constexpr auto get_kind() const -> Types::Result::Kind { return kind; }
  auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name.get_view();
  }
  constexpr auto get_payload() const -> const Model::Pack& {
    return payload.get();
  }

  auto equals(const Tetrodotoxin::Library::Language::Constant& rhs) const
      -> Bool override;

 private:
  Result(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Result& type,
      Types::Result::Kind kind,
      Model::Pack& payload,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Result& type,
      Types::Result::Kind kind,
      Model::Pack& payload) -> Perimortem::Core::Option<Result&>;

  const Types::Result& type;
  Types::Result::Kind kind;
  Tetrodotoxin::Source::PackReference<Model::Pack> payload;
  Perimortem::Memory::Managed::Bytes name;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
