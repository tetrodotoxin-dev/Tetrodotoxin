// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/terminal/llvm/module/body.hpp"
#include "tetrodotoxin/source/layout.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/callable.hpp"
#include "tetrodotoxin/source/pack.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Emission {

// States owns literal values, short circuit flow, sum states, and propagation
// for one native Body.
class States {
 public:
  enum class Logical : U8 {
    And,
    Or,
  };

  // Logic retains the source and merge blocks required after the right input
  // of one short circuit operation lowers.
  class Logic {
   public:
    constexpr Logic(LLVMBasicBlockRef left, LLVMBasicBlockRef merge)
        : left(left), merge(merge) {}

    constexpr auto get_left() const -> LLVMBasicBlockRef { return left; }

    constexpr auto get_merge() const -> LLVMBasicBlockRef { return merge; }

   private:
    LLVMBasicBlockRef left;
    LLVMBasicBlockRef merge;
  };

  // Choice retains one selected value and the two blocks needed to merge a
  // separately lowered fallback.
  class Choice {
   public:
    constexpr Choice(
        LLVMValueRef selected,
        LLVMBasicBlockRef selected_end,
        LLVMBasicBlockRef merge)
        : selected(selected), selected_end(selected_end), merge(merge) {}

    constexpr auto get_selected() const -> LLVMValueRef { return selected; }

    constexpr auto get_selected_end() const -> LLVMBasicBlockRef {
      return selected_end;
    }

    constexpr auto get_merge() const -> LLVMBasicBlockRef { return merge; }

   private:
    LLVMValueRef selected;
    LLVMBasicBlockRef selected_end;
    LLVMBasicBlockRef merge;
  };

  constexpr States(Module::Body& body) : body(body) {}
  States(const States&) = delete;
  States(States&&) = delete;
  auto operator=(const States&) -> States& = delete;
  auto operator=(States&&) -> States& = delete;

  constexpr auto get_program() const -> Module::Program& {
    return body.get_program();
  }

  auto unsigned_value(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      U64 value) const -> Bool;
  auto signed_value(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      S64 value) const -> Bool;
  auto real_value(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      R64 value) const -> Bool;
  auto bytes_value(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      Perimortem::Core::View::Bytes value,
      Perimortem::Core::Option<const Tetrodotoxin::Language::Resource&>
          resource = {}) const -> Bool;
  auto object_value(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result) const -> Bool;
  auto enumeration_name(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Source::Type& result_type,
      LLVMValueRef value,
      Perimortem::Core::View::Vector<U64> values,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names) const
      -> Bool;
  auto logical_not(
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& operand) const
      -> Bool;
  auto begin_logic(
      Logical operation,
      const Tetrodotoxin::Library::Language::Model::Pack& left) const
      -> Perimortem::Core::Option<Logic>;
  auto end_logic(
      Logic state,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& left,
      const Tetrodotoxin::Library::Language::Model::Pack& right) const -> Bool;
  auto absent(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result) const -> Bool;
  auto present(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Source::Type& element,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& payload) const
      -> Bool;
  auto result(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& payload) const
      -> Bool;
  auto begin_unwrap(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Source::Type& element,
      const Tetrodotoxin::Library::Language::Model::Pack& option) const
      -> Perimortem::Core::Option<Choice>;
  auto end_unwrap(
      Choice state,
      const Tetrodotoxin::Source::Type& element,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& fallback) const
      -> Bool;
  auto propagate_option(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Source::Type& element,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& option,
      const Tetrodotoxin::Library::Language::Model::Pack& escape) const -> Bool;
  auto propagate_flag(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& flag,
      const Tetrodotoxin::Library::Language::Model::Pack& escape) const -> Bool;
  auto propagate_result(
      const Tetrodotoxin::Source::Type& carrier,
      const Tetrodotoxin::Source::Type& value,
      const Tetrodotoxin::Source::Type& error,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Library::Language::Model::Pack& source,
      const Tetrodotoxin::Library::Language::Model::Pack& escape) const -> Bool;

 private:
  Module::Body& body;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Emission
