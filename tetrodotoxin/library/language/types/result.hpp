// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Result is an inline value sum. Exactly one alternative is live. Raw value or
// error flow constructs the matching state, while propagation continues with
// the value and requires the enclosing Function to receive the error.
class Result : public Model::Type {
 public:
  enum class Kind : U8 {
    Value,
    Error,
  };

  TTX_CONTRACT(Result, Model::Type);

  constexpr Result(
      Perimortem::Core::View::Bytes name,
      const Model::Type& value,
      const Model::Type& error,
      const Model::Types::Flag& flag)
      : name(name), value(value), error(error), flag(flag) {}

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  constexpr auto get_propagated_type() const
      -> Perimortem::Core::Option<const Model::Type&> override {
    return value;
  }

  constexpr auto get_propagated_error_type() const
      -> Perimortem::Core::Option<const Model::Type&> override {
    return error;
  }

  auto fold_propagation(Model::Pack& source) const -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Model::Pack&>, Bool> override;

  auto accepts(const Model::Pack& source) const -> Bool override;

  auto create_fitted(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& source) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto validate_layout(Tetrodotoxin::Source::Lexical::Cursor& cursor) const -> Bool override;

  constexpr auto get_value_type() const -> const Model::Type& { return value; }
  constexpr auto get_error_type() const -> const Model::Type& { return error; }

  constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> override {
    auto selected = value.get_declaration_anchor();
    return selected ? selected : error.get_declaration_anchor();
  }
  constexpr auto get_flag_type() const -> const Model::Types::Flag& {
    return flag;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& value;
  const Model::Type& error;
  const Model::Types::Flag& flag;
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Carries one value or one error as an explicit handled result."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
