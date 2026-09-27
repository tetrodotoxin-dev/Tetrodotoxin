// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Option is one nonnullable optional value Type. It specializes the receiving
// Type protocol to turn produced flow into one of its two runtime states:
//
// Pack with no values  to  Option target fit  to  absent Option[T]
// absent Option[T]     to  postfix question   to  return a Pack with no values
// absent Option[T]     to  postfix bang       to  Pack containing default(T)
//
// The Pack is empty only on the flow edge. Option and its element always keep
// nonempty Type Layouts, so absence never erases either semantic Type.
class Option : public Model::Type {
 public:
  enum class Kind : U8 {
    Absent,
    Present,
  };

  TTX_CONTRACT(Option, Model::Type);

  constexpr Option(
      Perimortem::Core::View::Bytes name,
      const Model::Type& element,
      const Model::Types::Flag& flag)
      : name(name), element(element), flag(flag) {}

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  constexpr auto get_propagated_type() const
      -> Perimortem::Core::Option<const Model::Type&> override {
    return element;
  }

  auto fold_propagation(Model::Pack& source) const -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Model::Pack&>, Bool> override;

  auto accepts(const Model::Pack& source) const -> Bool override;

  auto create_fitted(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& source) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto validate_layout(Tetrodotoxin::Source::Lexical::Cursor& cursor) const -> Bool override;

  constexpr auto get_element_type() const -> const Model::Type& {
    return element;
  }

  constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> override {
    return element.get_declaration_anchor();
  }

  constexpr auto get_flag_type() const -> const Model::Types::Flag& {
    return flag;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& element;
  const Model::Types::Flag& flag;
  static constexpr Tetrodotoxin::Source::Documentations::Comment documentation{
    "Carries either no value or one exact payload value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
