// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/library/language/types/implementation.hpp"
#include "tetrodotoxin/terminal/abi/representation/type.hpp"
#include "tetrodotoxin/terminal/llvm/module/emission.hpp"
#include "tetrodotoxin/source/layout.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/pack.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Module {

// Carriers retains the LLVM representation selected by each exact source Type.
// Library owners reserve recursive identities and complete only their own
// physical facts through the LLVM execution owners.
class Carriers {
 public:
  // Kind describes the completed physical carrier selected by the source Type
  // owner. Consumers can inspect this target fact without rediscovering the
  // concrete declaration that contributed it.
  using Kind = Tetrodotoxin::Terminal::Abi::Representation::Type::Kind;

  auto reserve(Emission& program, const Tetrodotoxin::Source::Type& type, Kind kind) const
      -> Perimortem::Core::Option<Bool>;

  auto begin_completion(Emission& program, const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<Bool>;

  auto complete(Emission& program, const Tetrodotoxin::Source::Type& type, Kind kind)
      const -> Bool;

  auto get_type(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<LLVMTypeRef>;

  auto get_payload(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<LLVMTypeRef>;

  auto get_kind(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<Kind>;

  auto get_width(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<Count>;

  auto get_element(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Type&>;

  auto get_flag(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Type&>;

  auto get_error(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Type&>;

  auto get_extent(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<Count>;

  auto get_fields(const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Layout&>;

  auto get_field_index(const Tetrodotoxin::Source::Addressable& field) const
      -> Perimortem::Core::Option<Count>;

  auto get_field_host(const Tetrodotoxin::Source::Addressable& field) const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Type&>;

  auto is_real(const Tetrodotoxin::Source::Type& type) const -> Bool;

  auto is_signed(const Tetrodotoxin::Source::Type& type) const -> Bool;

  auto is_flag(const Tetrodotoxin::Source::Type& type) const -> Bool;

  auto is_object(const Tetrodotoxin::Source::Type& type) const -> Bool;

  auto zero(Emission& program, const Tetrodotoxin::Source::Type& type) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto owns_resources(const Tetrodotoxin::Source::Type& type) const -> Bool;

  auto retain(Emission& body, const Tetrodotoxin::Source::Type& type, LLVMValueRef value)
      const -> Bool;

  auto release(Emission& body, const Tetrodotoxin::Source::Type& type, LLVMValueRef value)
      const -> Bool;

  auto select_result(
      Emission& body,
      const Tetrodotoxin::Source::Type& type,
      LLVMValueRef value,
      Bool value_selected) const -> Perimortem::Core::Option<LLVMValueRef>;

  auto assemble(
      Emission& body,
      const Tetrodotoxin::Source::Type& type,
      Perimortem::Core::View::Vector<LLVMValueRef> elements) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto fit_and_assemble(
      Emission& body,
      const Tetrodotoxin::Source::Type& type,
      const Tetrodotoxin::Library::Language::Model::Pack& source,
      Perimortem::Core::View::Vector<LLVMValueRef> elements) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto fit(
      Emission& body,
      const Tetrodotoxin::Library::Language::Model::Pack& source,
      const Tetrodotoxin::Source::Layout& target,
      Perimortem::Core::View::Vector<LLVMValueRef> values) const
      -> Perimortem::Core::Option<
          Perimortem::Memory::Dynamic::Vector<LLVMValueRef>>;

  auto construct(
      Emission& body,
      const Tetrodotoxin::Source::Type& type,
      Perimortem::Core::View::Vector<LLVMValueRef> values) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto get_object_descriptor(Emission& program, const Tetrodotoxin::Source::Type& type)
      const -> Perimortem::Core::Option<LLVMValueRef>;

 private:
  enum class Phase : U8 {
    Reserved,
    Completing,
    Complete,
  };

  struct Carrier {
    enum class Property : U8 {
      Real = 1,
      Signed = 2,
      Flag = 4,
    };

    constexpr auto has(Property property) const -> Bool {
      return Bool(properties & U8(property));
    }

    constexpr auto set(Property property, Bool value) -> void {
      if (value) {
        properties |= U8(property);
      }
    }

    Kind kind;
    Phase phase = Phase::Reserved;
    Perimortem::Core::Option<LLVMTypeRef> native;
    Perimortem::Core::Option<LLVMTypeRef> payload;
    Perimortem::Core::Option<LLVMValueRef> finalizer;
    Perimortem::Core::Option<LLVMValueRef> descriptor;
    Perimortem::Core::Option<const Tetrodotoxin::Source::Type&> element;
    Perimortem::Core::Option<const Tetrodotoxin::Source::Type&> error;
    Perimortem::Core::Option<const Tetrodotoxin::Source::Type&> flag;
    Perimortem::Core::Option<const Tetrodotoxin::Source::Layout&> fields;
    Count extent = 0;
    U8 properties = 0;
    Count width = 0;
  };

  auto publish(Emission& program, const Tetrodotoxin::Source::Type& type, Carrier carrier)
      const -> Perimortem::Core::Option<Bool>;

  auto select_completion(
      Emission& program,
      const Tetrodotoxin::Source::Type& type,
      Kind kind) const -> Perimortem::Core::Option<Carrier&>;

  auto complete_contiguous(
      Emission& program,
      const Tetrodotoxin::Source::Type& type,
      const Tetrodotoxin::Source::Type& element,
      Kind kind) const -> Bool;

  auto get_implementation_projection(
      Emission& program,
      const Tetrodotoxin::Source::Type& candidate) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto complete_aggregate(
      Emission& program,
      const Tetrodotoxin::Source::Type& type,
      const Tetrodotoxin::Source::Layout& fields,
      Kind kind) const -> Bool;

  auto fit_values(
      Emission& body,
      const Tetrodotoxin::Library::Language::Model::Pack& source,
      const Tetrodotoxin::Source::Layout& target,
      Perimortem::Core::View::Vector<LLVMValueRef> values) const
      -> Perimortem::Core::Option<
          Perimortem::Memory::Dynamic::Vector<LLVMValueRef>>;

  auto assemble_result(
      Emission& body,
      const Tetrodotoxin::Source::Type& type,
      const Tetrodotoxin::Source::Type& alternative,
      Bool value_selected,
      Perimortem::Core::View::Vector<LLVMValueRef> elements) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto owns_resources(
      const Tetrodotoxin::Source::Type& type,
      Perimortem::Memory::Dynamic::Vector<
          Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>>& active) const
      -> Bool;

  mutable Perimortem::Memory::Dynamic::Map<const Tetrodotoxin::Source::Type*, Carrier>
      carriers;
  mutable Perimortem::Memory::Dynamic::
      Map<const Tetrodotoxin::Source::Addressable*, Count>
          field_indices;
  mutable Perimortem::Memory::Dynamic::
      Map<const Tetrodotoxin::Source::Addressable*, const Tetrodotoxin::Source::Type*>
          field_hosts;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Module
