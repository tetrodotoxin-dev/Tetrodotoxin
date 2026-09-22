// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/terminal/llvm/module/emission.hpp"
#include "tetrodotoxin/terminal/llvm/module/program.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/callable.hpp"
#include "tetrodotoxin/source/pack.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Module {

// Body owns every mutable LLVM fact for one executable Library Body. Module
// facts remain on Program while this transaction owns values, local addresses,
// insertion state, and scoped lifetime records.
class Body : public Emission {
 public:
  using NativeValues = Perimortem::Memory::Dynamic::Vector<LLVMValueRef>;

  class TargetAddress {
   public:
    constexpr TargetAddress(const Tetrodotoxin::Source::Type& type, LLVMValueRef address)
        : type(type), address(address) {}

    constexpr auto get_type() const -> const Tetrodotoxin::Source::Type& {
      return type.get();
    }

    constexpr auto get_address() const -> LLVMValueRef { return address; }

   private:
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type> type;
    LLVMValueRef address;
  };

  class IndexedTarget {
   public:
    constexpr IndexedTarget(
        const Tetrodotoxin::Source::Type& type,
        LLVMTypeRef native_type,
        LLVMValueRef data,
        LLVMValueRef length,
        LLVMValueRef first,
        Perimortem::Core::Option<Count> range_size)
        : type(type),
          native_type(native_type),
          data(data),
          length(length),
          first(first),
          range_size(range_size) {}

    constexpr auto get_type() const -> const Tetrodotoxin::Source::Type& {
      return type.get();
    }

    constexpr auto get_native_type() const -> LLVMTypeRef {
      return native_type;
    }

    constexpr auto get_data() const -> LLVMValueRef { return data; }

    constexpr auto get_length() const -> LLVMValueRef { return length; }

    constexpr auto get_first() const -> LLVMValueRef { return first; }

    constexpr auto get_range_size() const -> Perimortem::Core::Option<Count> {
      return range_size;
    }

   private:
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type> type;
    LLVMTypeRef native_type;
    LLVMValueRef data;
    LLVMValueRef length;
    LLVMValueRef first;
    Perimortem::Core::Option<Count> range_size;
  };

  class BlockScope {
   public:
    constexpr BlockScope(
        const Tetrodotoxin::Source::Abstract& owner,
        Count storage_depth)
        : owner(owner), storage_depth(storage_depth) {}

    constexpr auto get_owner() const -> const Tetrodotoxin::Source::Abstract& {
      return owner.get();
    }

    constexpr auto get_storage_depth() const -> Count { return storage_depth; }

   private:
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract> owner;
    Count storage_depth;
  };

  class LoopTargets {
   public:
    constexpr LoopTargets(
        const Tetrodotoxin::Source::Abstract& owner,
        LLVMBasicBlockRef break_target,
        LLVMBasicBlockRef continue_target,
        Count storage_depth,
        Count lifetime_depth)
        : owner(owner),
          break_target(break_target),
          continue_target(continue_target),
          storage_depth(storage_depth),
          lifetime_depth(lifetime_depth) {}

    constexpr auto get_owner() const -> const Tetrodotoxin::Source::Abstract& {
      return owner.get();
    }

    constexpr auto get_break_target() const -> LLVMBasicBlockRef {
      return break_target;
    }

    constexpr auto get_continue_target() const -> LLVMBasicBlockRef {
      return continue_target;
    }

    constexpr auto get_storage_depth() const -> Count { return storage_depth; }

    // Continue preserves storage owned by the loop input, while every path
    // through done releases back to the surrounding lifetime depth.
    constexpr auto get_lifetime_depth() const -> Count {
      return lifetime_depth;
    }

   private:
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract> owner;
    LLVMBasicBlockRef break_target;
    LLVMBasicBlockRef continue_target;
    Count storage_depth;
    Count lifetime_depth;
  };

  Body(
      Program& program,
      const Tetrodotoxin::Source::Abstract& owner,
      LLVMValueRef function,
      Perimortem::Core::Option<const Tetrodotoxin::Source::Callable&> callable = {},
      Perimortem::Core::Option<LLVMValueRef> sret = {},
      Perimortem::Core::Option<LLVMTypeRef> sret_type = {});

  ~Body();

  constexpr auto get_program() const -> Program& { return program; }

  constexpr auto get_owner() const -> const Tetrodotoxin::Source::Abstract& {
    return owner.get();
  }

  auto get_builder() const -> LLVMBuilderRef;

  auto get_function() const -> LLVMValueRef;

  auto get_callable() const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Callable&>;

  auto get_sret() const -> Perimortem::Core::Option<LLVMValueRef>;

  auto get_sret_type() const -> Perimortem::Core::Option<LLVMTypeRef>;

  auto find_values(const Tetrodotoxin::Source::Pack& pack) const
      -> Perimortem::Core::Option<const NativeValues&>;

  auto find_value(const Tetrodotoxin::Source::Pack& pack) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto publish_values(
      const Tetrodotoxin::Source::Pack& pack,
      Perimortem::Core::View::Vector<LLVMValueRef> native) -> Bool;

  auto find_address(const Tetrodotoxin::Source::Addressable& addressable) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto publish_address(
      const Tetrodotoxin::Source::Addressable& addressable,
      LLVMValueRef value) -> Bool;

  auto find_target_address(const Tetrodotoxin::Source::Pack& pack) const
      -> Perimortem::Core::Option<const TargetAddress&>;

  auto publish_target_address(
      const Tetrodotoxin::Source::Pack& pack,
      const Tetrodotoxin::Source::Type& type,
      LLVMValueRef address) -> Bool;

  auto find_indexed_target(const Tetrodotoxin::Source::Pack& pack) const
      -> Perimortem::Core::Option<const IndexedTarget&>;

  auto publish_indexed_target(
      const Tetrodotoxin::Source::Pack& pack,
      const Tetrodotoxin::Source::Type& type,
      LLVMTypeRef native_type,
      LLVMValueRef data,
      LLVMValueRef length,
      LLVMValueRef first,
      Perimortem::Core::Option<Count> range_size) -> Bool;

  auto find_selection(const Tetrodotoxin::Source::Pack& pack) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto publish_selection(const Tetrodotoxin::Source::Pack& pack, LLVMValueRef value)
      -> Bool;

  auto get_storage_depth() const -> Count;

  auto register_storage(const Tetrodotoxin::Source::Type& type, LLVMValueRef address)
      -> Bool;

  auto resize_storage(Count size) -> Bool;

  auto mark_owned(const Tetrodotoxin::Source::Type& type, LLVMValueRef value) -> void;

  auto take_owned(LLVMValueRef value) -> Bool;

  auto acquire(const Tetrodotoxin::Source::Type& type, LLVMValueRef value) -> Bool;

  auto emit_storage_cleanup(Count first) -> Bool;

  auto emit_temporary_cleanup() -> Bool;

  auto clear_temporary_cleanup() -> Bool;

  auto create_return(Perimortem::Core::Option<LLVMValueRef> value = {}) -> Bool;

  auto push_block_scope(const Tetrodotoxin::Source::Abstract& owner) -> Bool;

  auto take_block_scope(const Tetrodotoxin::Source::Abstract& owner)
      -> Perimortem::Core::Option<BlockScope>;

  auto publish_loop(
      const Tetrodotoxin::Source::Abstract& owner,
      LLVMBasicBlockRef break_target,
      LLVMBasicBlockRef continue_target,
      Count lifetime_depth) -> Bool;

  auto find_loop(const Tetrodotoxin::Source::Abstract& owner) const
      -> Perimortem::Core::Option<const LoopTargets&>;

  auto remove_loop(const Tetrodotoxin::Source::Abstract& owner) -> Bool;

  auto create_entry_alloca(LLVMTypeRef type, Perimortem::Core::View::Bytes name)
      -> LLVMValueRef;

  auto get_debug_scope() const -> Perimortem::Core::Option<LLVMMetadataRef>;

  auto set_debug_scope(Perimortem::Core::Option<LLVMMetadataRef> scope) -> void;

  auto push_debug_scope(LLVMMetadataRef scope) -> Bool;

  auto pop_debug_scope() -> Bool;

 private:
  Program& program;
  Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract> owner;
  LLVMValueRef function;
  Perimortem::Core::Option<const Tetrodotoxin::Source::Callable&> callable;
  Perimortem::Core::Option<LLVMValueRef> sret;
  Perimortem::Core::Option<LLVMTypeRef> sret_type;
  LLVMBuilderRef builder;
  Perimortem::Memory::Dynamic::Map<const Tetrodotoxin::Source::Pack*, NativeValues>
      values;
  Perimortem::Memory::Dynamic::Map<const Tetrodotoxin::Source::Addressable*, LLVMValueRef>
      addresses;
  Perimortem::Memory::Dynamic::Map<const Tetrodotoxin::Source::Pack*, TargetAddress>
      target_addresses;
  Perimortem::Memory::Dynamic::Map<const Tetrodotoxin::Source::Pack*, IndexedTarget>
      indexed_targets;
  Perimortem::Memory::Dynamic::Map<const Tetrodotoxin::Source::Pack*, LLVMValueRef>
      selections;
  Perimortem::Memory::Dynamic::Vector<BlockScope> block_scopes;
  Perimortem::Memory::Dynamic::Vector<LoopTargets> loops;
  struct OwnedStorage {
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type> type;
    LLVMValueRef address;
  };
  struct OwnedValue {
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type> type;
    LLVMValueRef value;
  };
  Perimortem::Memory::Dynamic::Vector<OwnedStorage> owned_storages;
  Perimortem::Memory::Dynamic::Vector<OwnedValue> owned_values;
  Perimortem::Memory::Dynamic::Vector<LLVMMetadataRef> debug_scopes;
  Perimortem::Core::Option<LLVMMetadataRef> debug_scope;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Module
