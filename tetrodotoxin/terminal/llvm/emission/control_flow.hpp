// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/terminal/llvm/module/body.hpp"
#include "tetrodotoxin/source/layout.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/callable.hpp"
#include "tetrodotoxin/source/pack.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Emission {

// ControlFlow owns native control transfer, iteration, lexical scopes, and
// debug positions for one Library Function.
class ControlFlow {
 public:
  enum class LoopAction : U8 {
    Break,
    Continue,
  };

  // Branch retains alternate and merge blocks while its semantic owner lowers
  // the body and optional alternate Statement.
  class Branch {
   public:
    enum class Property : U8 {
      AlternateStarted = 1,
      BodyReaches = 2,
    };

    constexpr Branch(LLVMBasicBlockRef alternate, LLVMBasicBlockRef done)
        : alternate(alternate), done(done) {}

    constexpr auto get_alternate() const -> LLVMBasicBlockRef {
      return alternate;
    }

    constexpr auto get_done() const -> LLVMBasicBlockRef { return done; }

    constexpr auto has_alternate() const -> Bool {
      return Bool(properties & U8(Property::AlternateStarted));
    }

    constexpr auto body_reaches_done() const -> Bool {
      return Bool(properties & U8(Property::BodyReaches));
    }

    constexpr auto begin_alternate(Bool reaches_done) -> void {
      properties |= U8(Property::AlternateStarted);
      if (reaches_done) {
        properties |= U8(Property::BodyReaches);
      }
    }

   private:
    LLVMBasicBlockRef alternate;
    LLVMBasicBlockRef done;
    U8 properties = 0;
  };

  // Match retains the selected input and final merge block while Match lowers
  // its authored cases in order.
  class Match {
   public:
    constexpr Match(LLVMValueRef input, LLVMBasicBlockRef done)
        : input(input), done(done) {}

    constexpr auto get_input() const -> LLVMValueRef { return input; }

    constexpr auto get_done() const -> LLVMBasicBlockRef { return done; }

    constexpr auto reaches_done() const -> Bool { return reaches; }

    constexpr auto set_reaches_done() -> void { reaches = True; }

   private:
    LLVMValueRef input;
    LLVMBasicBlockRef done;
    Bool reaches = False;
  };

  // MatchCase retains one case cleanup boundary and the next comparison block.
  class MatchCase {
   public:
    constexpr MatchCase(
        Count storage_depth,
        Perimortem::Core::Option<LLVMBasicBlockRef> next)
        : storage_depth(storage_depth), next(next) {}

    constexpr auto get_storage_depth() const -> Count { return storage_depth; }

    constexpr auto get_next() const
        -> Perimortem::Core::Option<LLVMBasicBlockRef> {
      return next;
    }

   private:
    Count storage_depth;
    Perimortem::Core::Option<LLVMBasicBlockRef> next;
  };

  constexpr ControlFlow(Module::Body& body) : body(body) {}
  ControlFlow(const ControlFlow&) = delete;
  ControlFlow(ControlFlow&&) = delete;
  auto operator=(const ControlFlow&) -> ControlFlow& = delete;
  auto operator=(ControlFlow&&) -> ControlFlow& = delete;

  constexpr auto get_program() const -> Module::Program& {
    return body.get_program();
  }

  auto return_values(
      const Tetrodotoxin::Library::Language::Model::Pack& values) const -> Bool;
  auto escape_values(
      const Tetrodotoxin::Library::Language::Model::Pack& values) const -> Bool;
  auto leave_loop(LoopAction action, const Tetrodotoxin::Source::Abstract& target) const
      -> Bool;
  auto begin_branch(
      const Tetrodotoxin::Library::Language::Model::Pack& condition) const
      -> Perimortem::Core::Option<Branch>;
  auto begin_alternate(Branch& state) const -> Bool;
  auto end_branch(Branch state) const -> Bool;
  auto begin_while(const Tetrodotoxin::Source::Abstract& owner) const -> Bool;
  auto select_while(
      const Tetrodotoxin::Source::Abstract& owner,
      const Tetrodotoxin::Library::Language::Model::Pack& condition) const
      -> Bool;
  auto end_while(const Tetrodotoxin::Source::Abstract& owner) const -> Bool;
  auto begin_sequence(
      const Tetrodotoxin::Source::Abstract& owner,
      const Tetrodotoxin::Source::Addressable& binding,
      const Tetrodotoxin::Source::Type& input_type,
      const Tetrodotoxin::Library::Language::Model::Pack& input) const -> Bool;
  auto begin_enumeration(
      const Tetrodotoxin::Source::Abstract& owner,
      const Tetrodotoxin::Source::Layout& bindings,
      Perimortem::Core::View::Vector<U64> values,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names) const
      -> Bool;
  auto end_iteration(const Tetrodotoxin::Source::Abstract& owner) const -> Bool;
  auto begin_match(const Tetrodotoxin::Library::Language::Model::Pack& input)
      const -> Perimortem::Core::Option<Match>;
  auto begin_constant_case(
      Match& state,
      const Tetrodotoxin::Library::Language::Model::Pack& constant) const
      -> Perimortem::Core::Option<MatchCase>;
  auto begin_value_case(
      Match& state,
      const Tetrodotoxin::Source::Addressable& payload,
      Tetrodotoxin::Source::Lexical::Anchor anchor) const -> Perimortem::Core::Option<MatchCase>;
  auto end_match_case(Match& state, MatchCase selected) const -> Bool;
  auto begin_default_case() const -> MatchCase;
  auto end_match(Match state, Bool unmatched_reaches_next) const -> Bool;
  auto begin_function(
      const Tetrodotoxin::Source::Callable& callable,
      const Tetrodotoxin::Language::Definition& definition) const -> Bool;
  auto parameter(
      const Tetrodotoxin::Source::Addressable& parameter,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Count index) const -> Bool;
  auto end_function() const -> Bool;
  auto begin_block(
      const Tetrodotoxin::Source::Abstract& block,
      Tetrodotoxin::Source::Lexical::Anchor anchor) const -> Bool;
  auto end_block(const Tetrodotoxin::Source::Abstract& block) const -> Bool;
  auto statement(Tetrodotoxin::Source::Lexical::Anchor anchor) const -> Bool;
  auto local(const Tetrodotoxin::Source::Addressable& local, Tetrodotoxin::Source::Lexical::Anchor anchor)
      const -> Bool;
  auto has_full_debug() const -> Bool;
  auto constant_local(
      const Tetrodotoxin::Source::Addressable& local,
      const Tetrodotoxin::Library::Language::Model::Pack& value,
      Tetrodotoxin::Source::Lexical::Anchor anchor) const -> Bool;
  auto end_statement() const -> Bool;
  auto bind_local(
      const Tetrodotoxin::Source::Addressable& local,
      const Tetrodotoxin::Library::Language::Model::Pack& value) const -> Bool;

 private:
  Module::Body& body;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Emission
