// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/terminal/llvm/module/emission.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/callable.hpp"
#include "tetrodotoxin/source/pack.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Module {

class Functions {
 public:
  class Lowering {
   public:
    constexpr Lowering(
        LLVMValueRef function,
        const Tetrodotoxin::Source::Callable& callable,
        Perimortem::Core::Option<LLVMValueRef> sret,
        Perimortem::Core::Option<LLVMTypeRef> sret_type)
        : function(function),
          callable(callable),
          sret(sret),
          sret_type(sret_type) {}

    constexpr auto get_function() const -> LLVMValueRef { return function; }

    constexpr auto get_callable() const -> const Tetrodotoxin::Source::Callable& {
      return callable.get();
    }

    constexpr auto get_sret() const -> Perimortem::Core::Option<LLVMValueRef> {
      return sret;
    }

    constexpr auto get_sret_type() const
        -> Perimortem::Core::Option<LLVMTypeRef> {
      return sret_type;
    }

   private:
    LLVMValueRef function;
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Callable> callable;
    Perimortem::Core::Option<LLVMValueRef> sret;
    Perimortem::Core::Option<LLVMTypeRef> sret_type;
  };

  // ConstructionField is one target lowering fact supplied by the aggregate
  // Type that owns the Field order and default. It retains only original graph
  // identities. LLVM neither discovers Fields nor manufactures a semantic
  // construction model.
  class ConstructionField {
   public:
    constexpr ConstructionField(
        const Tetrodotoxin::Source::Addressable& field,
        const Tetrodotoxin::Library::Language::Model::Pack& fallback,
        Bool parameter)
        : field(field), fallback(fallback), parameter(parameter) {}

    constexpr auto get_field() const -> const Tetrodotoxin::Source::Addressable& {
      return field.get();
    }

    constexpr auto get_fallback() const
        -> const Tetrodotoxin::Library::Language::Model::Pack& {
      return fallback.get();
    }

    constexpr auto is_parameter() const -> Bool { return parameter; }

   private:
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable> field;
    Tetrodotoxin::Source::PackReference<
        const Tetrodotoxin::Library::Language::Model::Pack>
        fallback;
    Bool parameter;
  };

  auto reserve_function(
      Emission& program,
      const Tetrodotoxin::Source::Callable& callable,
      const Tetrodotoxin::Language::Definition& definition) const
      -> Perimortem::Core::Option<Bool>;

  auto reserve_foreign(
      Emission& program,
      const Tetrodotoxin::Source::Callable& callable,
      Perimortem::Core::View::Bytes abi,
      Perimortem::Core::View::Bytes symbol) const
      -> Perimortem::Core::Option<Bool>;

  auto reserve_construction(
      Emission& program,
      const Tetrodotoxin::Source::Type& owner,
      Bool provider,
      Perimortem::Core::View::Vector<
          Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable>> parameters)
      const -> Bool;

  auto complete_construction(Emission& program, const Tetrodotoxin::Source::Type& owner)
      const -> Bool;

  auto lower_construction(
      Emission& program,
      const Tetrodotoxin::Source::Type& owner,
      Perimortem::Core::View::Vector<ConstructionField> fields) const -> Bool;

  auto call_construction(
      Emission& body,
      const Tetrodotoxin::Library::Language::Model::Pack& result,
      const Tetrodotoxin::Source::Type& owner,
      const Tetrodotoxin::Library::Language::Model::Pack& arguments) const
      -> Bool;

  auto complete(Emission& program, const Tetrodotoxin::Source::Callable& callable) const
      -> Bool;

  auto begin_body(Emission& program, const Tetrodotoxin::Source::Callable& callable) const
      -> Perimortem::Core::Option<Lowering>;

  auto bind_parameters(Emission& body, const Tetrodotoxin::Source::Callable& callable)
      const -> Bool;

  auto end_body(Emission& body, const Tetrodotoxin::Source::Callable& callable) const
      -> Bool;

  auto append_call_arguments(
      Emission& body,
      const Tetrodotoxin::Source::Callable& callable,
      Count parameter,
      LLVMValueRef value,
      Perimortem::Memory::Dynamic::Vector<LLVMValueRef>& arguments) const
      -> Bool;

  auto decode_call_result(
      Emission& body,
      const Tetrodotoxin::Source::Callable& callable,
      LLVMValueRef value) const -> Perimortem::Core::Option<LLVMValueRef>;

  auto encode_return(
      Emission& body,
      const Tetrodotoxin::Source::Callable& callable,
      LLVMValueRef value) const -> Perimortem::Core::Option<LLVMValueRef>;

  auto find_function(const Tetrodotoxin::Source::Callable& callable) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto find_symbol(const Tetrodotoxin::Source::Callable& callable) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;

  auto find_sret_type(const Tetrodotoxin::Source::Callable& callable) const
      -> Perimortem::Core::Option<LLVMTypeRef>;

  auto get_indirect_parameters(const Tetrodotoxin::Source::Callable& callable) const
      -> Perimortem::Core::View::Vector<Bool>;

  auto get_foreign_callables() const -> Perimortem::Core::View::Vector<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Callable>>;

 private:
  enum class Kind : U8 {
    Function,
    External,
    Foreign,
  };

  class Record {
   public:
    Record(
        Kind kind,
        Perimortem::Core::View::Bytes abi = {},
        Perimortem::Core::View::Bytes symbol = {},
        Perimortem::Core::Option<const Tetrodotoxin::Language::Definition&>
            definition = {})
        : kind(kind), abi(abi), symbol(symbol), definition(definition) {}

    Kind kind;
    Perimortem::Core::View::Bytes abi;
    Perimortem::Core::View::Bytes symbol;
    Perimortem::Core::Option<const Tetrodotoxin::Language::Definition&>
        definition;
    Perimortem::Core::Option<LLVMValueRef> function;
    Perimortem::Core::Option<LLVMTypeRef> sret_type;
    Perimortem::Core::Option<LLVMTypeRef> result_type;
    Perimortem::Core::Option<LLVMTypeRef> result_abi_type;
    Perimortem::Memory::Dynamic::Vector<Bool> indirect_parameters;
    Perimortem::Memory::Dynamic::Vector<LLVMTypeRef> parameter_types;
    Perimortem::Memory::Dynamic::Vector<LLVMTypeRef> parameter_abi_types;
    Perimortem::Memory::Dynamic::Vector<Count> parameter_abi_counts;
  };

  class ConstructionRecord {
   public:
    ConstructionRecord(Bool provider, Perimortem::Core::View::Bytes symbol)
        : provider(provider), symbol(symbol) {}

    Bool provider;
    Perimortem::Core::View::Bytes symbol;
    Perimortem::Core::Option<LLVMValueRef> function;
    Perimortem::Core::Option<LLVMTypeRef> sret_type;
    Perimortem::Memory::Dynamic::Vector<
        Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable>>
        parameters;
    Perimortem::Memory::Dynamic::Vector<Bool> indirect_parameters;
  };

  auto reserve(
      Emission& program,
      const Tetrodotoxin::Source::Callable& callable,
      Record record) const -> Perimortem::Core::Option<Bool>;

  mutable Perimortem::Memory::Dynamic::Map<const Tetrodotoxin::Source::Callable*, Record>
      records;
  mutable Perimortem::Memory::Dynamic::
      Map<const Tetrodotoxin::Source::Type*, ConstructionRecord>
          constructions;
  mutable Perimortem::Memory::Dynamic::Vector<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Callable>>
      foreign_callables;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Module
