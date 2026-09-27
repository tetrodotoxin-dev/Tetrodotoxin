// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Object is the immutable empty value of one generated Object[T] storage Type.
// It carries no allocation while preserving the exact materialized Type.
class Object : public Tetrodotoxin::Library::Language::Constant {
 public:
  TTX_CONTRACT(Object, Tetrodotoxin::Library::Language::Constant);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Model::Type& type) -> Object& {
    return Constant::create_synthetic<Object>(
        domain, [&](auto anchor) -> Object { return Object(type, anchor); });
  }

  constexpr auto get_type() const -> const Model::Type& override {
    return type;
  }

  TTX_NAME("[]"_view);

  constexpr auto equals(const Tetrodotoxin::Library::Language::Constant& rhs)
      const -> Bool override {
    return has_same_type(rhs) && rhs.is<Object>();
  }

 private:
  constexpr Object(
      const Model::Type& type,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor)
      : Tetrodotoxin::Library::Language::Constant(anchor), type(type) {}

  const Model::Type& type;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
