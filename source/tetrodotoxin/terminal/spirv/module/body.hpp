// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/terminal/spirv/assembler/spir_v.hpp"
#include "tetrodotoxin/terminal/spirv/module/constants.hpp"
#include "tetrodotoxin/terminal/spirv/module/ids.hpp"
#include "tetrodotoxin/terminal/spirv/module/interface.hpp"
#include "tetrodotoxin/terminal/spirv/module/types.hpp"
#include "tetrodotoxin/terminal/spirv/request.hpp"
#include "tetrodotoxin/source/reference.hpp"

namespace Tetrodotoxin::Terminal::Spirv::Module {

// Body lowers the real Library Block selected by one Shader Stage. Request
// local values key the original semantic owners while SPIR V result ids carry
// only the physical data flow needed by the emitted function.
class Body {
 public:
  Body(
      Ids& ids,
      Types& types,
      Constants& constants,
      const Interface& interface,
      const Request& request)
      : ids(ids),
        types(types),
        constants(constants),
        interface(interface),
        request(request) {}

  auto prepare(const Interface::Stage& stage) -> Bool;
  auto emit(const Interface::Stage& stage, Assembler::SpirV& assembler) -> Bool;

 private:
  class Value {
   public:
    constexpr Value(
        const Tetrodotoxin::Source::Abstract& semantic,
        const Tetrodotoxin::Library::Language::Model::Type& type,
        U32 id)
        : semantic(semantic), type(type), id(id) {}

    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Abstract> semantic;
    Tetrodotoxin::Source::Reference<const Tetrodotoxin::Library::Language::Model::Type>
        type;
    U32 id;
  };

  auto prepare_pack(const Tetrodotoxin::Library::Language::Model::Pack& pack)
      -> Bool;
  auto prepare_expression(
      const Tetrodotoxin::Library::Language::Expression& expression) -> Bool;
  auto lower_pack(
      const Tetrodotoxin::Library::Language::Model::Pack& pack,
      const Tetrodotoxin::Library::Language::Model::Type& expected,
      Assembler::SpirV& assembler) -> Perimortem::Core::Option<Value>;
  auto lower_expression(
      const Tetrodotoxin::Library::Language::Expression& expression,
      Assembler::SpirV& assembler) -> Perimortem::Core::Option<Value>;
  auto lower_return(
      const Tetrodotoxin::Library::Language::Model::Pack& pack,
      const Interface::Stage& stage,
      Assembler::SpirV& assembler) -> Bool;
  auto find_value(const Tetrodotoxin::Source::Abstract& semantic) const
      -> Perimortem::Core::Option<Value>;
  auto retain_value(Value value) -> Bool;
  auto reject(
      const Tetrodotoxin::Source::Abstract& semantic,
      Perimortem::Core::View::Bytes message) const -> Bool;

  static auto select_source(
      const Tetrodotoxin::Library::Language::Model::Pack& pack,
      const Tetrodotoxin::Source::Layout& target,
      Count target_index) -> Perimortem::Core::Option<Count>;

  Ids& ids;
  Types& types;
  Constants& constants;
  const Interface& interface;
  const Request& request;
  Perimortem::Memory::Dynamic::Vector<Value> values;
};

}  // namespace Tetrodotoxin::Terminal::Spirv::Module
