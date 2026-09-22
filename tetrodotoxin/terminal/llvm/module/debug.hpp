// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/terminal/llvm/module/emission.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/callable.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Module {

// Debug owns authored source correlation for one Program transaction. Level
// selects policy while every retained LLVM metadata identity remains private to
// this owner.
class Debug {
 public:
  enum class Level : U8 {
    None,
    Line,
    Full,
  };

  constexpr Debug(Level level) : level(level) {}

  Debug(const Debug&) = delete;
  Debug(Debug&&) = delete;
  auto operator=(const Debug&) -> Debug& = delete;
  auto operator=(Debug&&) -> Debug& = delete;

  constexpr auto get_level() const -> Level { return level; }

  constexpr auto get_builder() const
      -> Perimortem::Core::Option<LLVMOpaqueDIBuilder&> {
    return builder;
  }

  constexpr auto get_file() const
      -> Perimortem::Core::Option<LLVMOpaqueMetadata&> {
    return file;
  }

  auto initialize(
      Emission& program,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text) -> Bool;

  auto release() -> void;

  auto finalize(Emission& program) -> Bool;

  auto find_type(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<LLVMMetadataRef>;

  auto publish_type(const Tetrodotoxin::Source::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto replace_type(const Tetrodotoxin::Source::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto find_payload(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<LLVMMetadataRef>;

  auto publish_payload(const Tetrodotoxin::Source::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto replace_payload(const Tetrodotoxin::Source::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto publish_enumerator(
      const Tetrodotoxin::Source::Type& type,
      LLVMMetadataRef metadata) -> Bool;

  auto get_enumerators(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::View::Vector<LLVMMetadataRef>;

  auto find_scope(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<LLVMMetadataRef>;

  auto publish_scope(const Tetrodotoxin::Source::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto replace_scope(const Tetrodotoxin::Source::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto get_scope_types() const -> Perimortem::Core::View::Vector<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>>;

  auto publish_member(const Tetrodotoxin::Source::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto get_members(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::View::Vector<LLVMMetadataRef>;

  auto type(
      const Tetrodotoxin::Source::Type& type,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> anchor) -> Bool;

  auto field(const Tetrodotoxin::Source::Addressable& field, Tetrodotoxin::Source::Lexical::Anchor anchor)
      -> Bool;

  auto signed_enumerator(
      Emission& program,
      const Tetrodotoxin::Source::Type& type,
      const Tetrodotoxin::Source::Abstract& enumerator,
      S64 value) -> Bool;

  auto unsigned_enumerator(
      Emission& program,
      const Tetrodotoxin::Source::Type& type,
      const Tetrodotoxin::Source::Abstract& enumerator,
      U64 value) -> Bool;

  auto global(
      Emission& program,
      const Tetrodotoxin::Source::Addressable& addressable,
      const Tetrodotoxin::Language::Definition& definition,
      Bool local,
      Bool defined) -> Bool;

  auto begin_function(
      Emission& body,
      const Tetrodotoxin::Source::Callable& callable,
      const Tetrodotoxin::Language::Definition& definition) -> Bool;

  auto parameter(
      Emission& body,
      const Tetrodotoxin::Source::Addressable& parameter,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Count index) -> Bool;

  auto end_function(Emission& body) -> Bool;

  auto begin_block(
      Emission& body,
      const Tetrodotoxin::Source::Abstract& block,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Bool;

  auto statement(Emission& body, Tetrodotoxin::Source::Lexical::Anchor anchor) -> Bool;

  auto local(
      Emission& body,
      const Tetrodotoxin::Source::Addressable& local,
      Tetrodotoxin::Source::Lexical::Anchor anchor) -> Bool;

  auto value(
      Emission& body,
      const Tetrodotoxin::Source::Addressable& local,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      LLVMValueRef value) -> Bool;

 private:
  Level level;
  Perimortem::Core::Option<LLVMOpaqueDIBuilder&> builder;
  Perimortem::Core::Option<LLVMOpaqueMetadata&> file;
  Perimortem::Memory::Dynamic::Map<const Tetrodotoxin::Source::Type*, LLVMMetadataRef>
      types;
  Perimortem::Memory::Dynamic::Map<const Tetrodotoxin::Source::Type*, LLVMMetadataRef>
      payloads;
  Perimortem::Memory::Dynamic::Map<
      const Tetrodotoxin::Source::Type*,
      Perimortem::Memory::Dynamic::Vector<LLVMMetadataRef>>
      enumerators;
  Perimortem::Memory::Dynamic::Map<const Tetrodotoxin::Source::Type*, LLVMMetadataRef>
      scopes;
  Perimortem::Memory::Dynamic::Vector<
      Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>>
      scope_types;
  Perimortem::Memory::Dynamic::Map<
      const Tetrodotoxin::Source::Type*,
      Perimortem::Memory::Dynamic::Vector<LLVMMetadataRef>>
      members;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Module
