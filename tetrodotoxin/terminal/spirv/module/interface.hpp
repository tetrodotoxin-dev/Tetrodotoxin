// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/attribute.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/shader/language/program.hpp"
#include "tetrodotoxin/terminal/spirv/assembler/spir_v.hpp"
#include "tetrodotoxin/terminal/spirv/module/ids.hpp"
#include "tetrodotoxin/terminal/spirv/module/types.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Terminal::Spirv::Module {

// Interface derives entry point variables and decorations from one Shader
// Program and its completed Render agreement. The records are request local
// target facts rather than another interface graph.
class Interface {
 public:
  class Variable {
   public:
    constexpr Variable(
        const Tetrodotoxin::Source::Abstract& semantic,
        const Tetrodotoxin::Library::Language::Model::Type& type,
        Perimortem::Core::View::Bytes name,
        Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
            attributes,
        Assembler::SpirV::StorageClass storage,
        U32 id,
        Count push_index = Count(-1),
        U32 push_index_id = 0,
        Count location = Count(-1))
        : semantic(semantic),
          type(type),
          name(name),
          attributes(attributes),
          storage(storage),
          id(id),
          push_index(push_index),
          push_index_id(push_index_id),
          location(location) {}

    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract> semantic;
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Library::Language::Model::Type>
        type;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
        attributes;
    Assembler::SpirV::StorageClass storage;
    U32 id;
    Count push_index;
    U32 push_index_id;
    Count location;
  };

  class Stage {
   public:
    Stage(
        Perimortem::Memory::Allocator::Arena& arena,
        const Tetrodotoxin::Library::Language::Function& function,
        Assembler::SpirV::ExecutionModel model,
        U32 id)
        : function(function),
          model(model),
          id(id),
          inputs(arena),
          outputs(arena) {}

    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Library::Language::Function>
        function;
    Assembler::SpirV::ExecutionModel model;
    U32 id;
    Perimortem::Memory::Managed::Vector<Variable> inputs;
    Perimortem::Memory::Managed::Vector<Variable> outputs;
  };

  Interface(Perimortem::Memory::Allocator::Arena& arena, Ids& ids, Types& types)
      : arena(arena), ids(ids), types(types), bindings(arena), stages(arena) {}

  auto prepare(const Tetrodotoxin::Shader::Language::Program& program) -> Bool;
  auto emit_entry_points(Assembler::SpirV& assembler) const -> void;
  auto emit_debug(Assembler::SpirV& assembler) const -> void;
  auto emit_annotations(Assembler::SpirV& assembler) const -> Bool;
  auto emit_types(Assembler::SpirV& assembler) const -> Bool;
  auto emit_globals(Assembler::SpirV& assembler) const -> Bool;

  auto get_binding_pointer(const Variable& binding, Assembler::SpirV& assembler)
      const -> Perimortem::Core::Option<U32>;

  constexpr auto get_stages() const -> Perimortem::Core::View::Vector<Stage*> {
    return stages;
  }

  constexpr auto get_bindings() const
      -> Perimortem::Core::View::Vector<Variable> {
    return bindings;
  }

  auto is_resource(const Tetrodotoxin::Source::Abstract& semantic) const -> Bool;

 private:
  auto prepare_variables(
      Stage& stage,
      const Tetrodotoxin::Library::Language::Model::Layout& layout,
      Assembler::SpirV::StorageClass storage) -> Bool;
  auto prepare_bindings(const Tetrodotoxin::Shader::Language::Program& program)
      -> Bool;
  static auto decorate(Assembler::SpirV& assembler, const Variable& variable)
      -> Bool;

  Perimortem::Memory::Allocator::Arena& arena;
  Ids& ids;
  Types& types;
  Perimortem::Memory::Managed::Vector<Variable> bindings;
  Perimortem::Memory::Managed::Vector<Stage*> stages;
  U32 push_type_id = 0;
  U32 push_pointer_id = 0;
  U32 push_variable_id = 0;
  U32 push_index_type_id = 0;
  Bool owns_push_index_type = False;
};

}  // namespace Tetrodotoxin::Terminal::Spirv::Module
