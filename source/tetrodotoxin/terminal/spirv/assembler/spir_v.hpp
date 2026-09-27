// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Tetrodotoxin::Terminal::Spirv::Assembler {

// Minimal SPIR V module writer.
//
// SPIR V is a stream of 32 bit little endian words, not a byte oriented opcode
// stream. A module starts with a five word header:
//
//   magic, version, generator, bound, schema
//
// Every instruction after that starts with one packed word:
//
//   high 16 bits = instruction word count
//   low  16 bits = opcode
//
// Operands follow as additional 32 bit words. Strings are UTF 8 bytes packed
// into words with a trailing null and zero padding. Result ids are module local
// names for types, constants, variables, labels, functions, and temporary
// values. The header bound is one greater than every id that can appear.
//
// The assembler writes words but does not validate section ordering or operand
// types. The selected SPIR V target owns those rules and this is just a basic
// bytecode emitter.
class SpirV {
 public:
  enum class Version : U32 {
    V1_0 = 0x00010000,
  };

  // Numeric opcode values come from the SPIR V grammar. The gaps are part of
  // the format since we don't support the entire SPIR V Spec yet.
  enum class Op : U16 {
    Nop = 0,                  // No operation.
    Undef = 1,                // Creates an undefined value of a type.
    SourceContinued = 2,      // Continues source language debug text.
    Source = 3,               // Describes the source language for debug tools.
    SourceExtension = 4,      // Names a source language extension.
    Name = 5,                 // Assigns a debug name to an id.
    MemberName = 6,           // Assigns a debug name to a struct member.
    String = 7,               // Creates a reusable debug string literal.
    Line = 8,                 // Associates following instructions with a line.
    Extension = 10,           // Requests a SPIR V extension.
    ExtInstImport = 11,       // Imports an extended instruction set.
    ExtInst = 12,             // Calls an extended instruction.
    MemoryModel = 14,         // Selects addressing and memory semantics.
    EntryPoint = 15,          // Publishes a callable shader entry function.
    ExecutionMode = 16,       // Adds Stage specific execution metadata.
    Capability = 17,          // Enables a group of SPIR V features.
    TypeVoid = 19,            // Defines the void type.
    TypeBool = 20,            // Defines the bool type.
    TypeInt = 21,             // Defines a signed or unsigned integer type.
    TypeFloat = 22,           // Defines a floating point type.
    TypeVector = 23,          // Defines a fixed width vector type.
    TypeImage = 25,           // Defines an opaque image resource type.
    TypeSampler = 26,         // Defines an opaque sampler resource type.
    TypeSampledImage = 27,    // Defines the image+sampler value used to sample.
    TypeArray = 28,           // Defines an array type.
    TypeStruct = 30,          // Defines a struct type.
    TypePointer = 32,         // Defines a pointer into a storage class.
    TypeFunction = 33,        // Defines a function signature type.
    ConstantTrue = 41,        // Creates the true value of a Bool type.
    ConstantFalse = 42,       // Creates the false value of a Bool type.
    Constant = 43,            // Creates a scalar constant.
    ConstantComposite = 44,   // Creates a vector/struct/array constant.
    Function = 54,            // Begins a function body.
    FunctionEnd = 56,         // Ends the current function body.
    Variable = 59,            // Declares storage for a pointer typed value.
    Load = 61,                // Reads through a pointer.
    Store = 62,               // Writes through a pointer.
    AccessChain = 65,         // Computes a pointer to a composite member.
    Decorate = 71,            // Attaches metadata to an id.
    MemberDecorate = 72,      // Attaches metadata to a struct member.
    VectorShuffle = 79,       // Builds a vector by selecting source lanes.
    CompositeConstruct = 80,  // Builds a composite from constituent ids.
    CompositeExtract = 81,    // Reads a member/lane out of a composite.
    ImageSampleImplicitLod = 87,  // Samples an image using implicit LOD.
    ConvertUToF = 112,            // Converts an unsigned integer to a Real.
    FConvert = 115,               // Converts one floating point width.
    FAdd = 129,                   // Floating point add.
    FSub = 131,                   // Floating point subtract.
    FMul = 133,                   // Floating point multiply.
    FDiv = 136,                   // Floating point divide.
    FRem = 140,                   // Floating point remainder.
    Label = 248,                  // Begins a basic block.
    Return = 253,                 // Returns from the current function.
  };

  enum class Capability : U32 {
    Shader = 1,
    Float64 = 10,
  };

  enum class AddressingModel : U32 {
    Logical = 0,
  };

  enum class MemoryModel : U32 {
    GLSL450 = 1,
  };

  enum class ExecutionModel : U32 {
    Vertex = 0,
    Fragment = 4,
  };

  enum class ExecutionMode : U32 {
    OriginUpperLeft = 7,
  };

  enum class Dim : U32 {
    D2 = 1,
  };

  enum class ImageFormat : U32 {
    Unknown = 0,
  };

  enum class StorageClass : U32 {
    UniformConstant = 0,
    Input = 1,
    Uniform = 2,
    Output = 3,
    Function = 7,
    PushConstant = 9,
  };

  enum class Decoration : U32 {
    Block = 2,
    BuiltIn = 11,
    Location = 30,
    Binding = 33,
    DescriptorSet = 34,
    Offset = 35,
  };

  // BuiltIn grows with the semantic contracts this Terminal can emit. Position
  // and VertexIndex form the current authored surface.
  enum class BuiltIn : U32 {
    Position = 0,
    VertexIndex = 42,
  };

  enum class FunctionControl : U32 {
    None = 0,
  };

  static constexpr U32 magic = 0x07230203;

  explicit SpirV(Perimortem::Memory::Dynamic::Bytes& words) : words(words) {}

  // Writes the five word module header. `bound` is one greater than the largest
  // result id the module may use, not the instruction count.
  auto begin_module(
      U32 bound,
      Version version = Version::V1_0,
      U32 generator = 0) -> void;

  auto patch_bound(U32 bound) -> Bool;

  // Low level writing primitives support the typed helpers below, which keep
  // each instruction word count paired with its opcode shape.
  auto word(U32 value) -> void;
  auto instruction(Op opcode, Count word_count) -> void;
  auto literal_string(Perimortem::Core::View::Bytes text) -> Count;

  // Logical layout helpers. SPIR V validators expect these groups in order:
  // capabilities, extensions/imports, memory model, entry points/execution
  // modes, debug names, annotations, type/global declarations, then functions.
  auto capability(Capability value) -> void;
  auto memory_model(AddressingModel addressing, MemoryModel memory) -> void;
  auto entry_point(
      ExecutionModel model,
      U32 function_id,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<U32> interface_ids = {}) -> void;
  auto execution_mode(U32 entry_point_id, ExecutionMode mode) -> void;

  // Debug names do not define ids. They annotate ids that may be declared
  // later, which lets the module producer emit names before the Type and
  // Function declarations.
  auto name(U32 target_id, Perimortem::Core::View::Bytes name) -> void;
  auto member_name(
      U32 target_id,
      U32 member_index,
      Perimortem::Core::View::Bytes name) -> void;

  // Decorations are semantic metadata consumed by APIs such as Vulkan:
  // locations, descriptor bindings, builtins, push constant block layout, and
  // byte offsets.
  auto decorate(
      U32 target_id,
      Decoration decoration,
      Perimortem::Core::Option<U32> value = {}) -> void;
  auto member_decorate(
      U32 target_id,
      U32 member_index,
      Decoration decoration,
      U32 value) -> void;

  // Type declarations produce ids for later instructions. SPIR V is strongly
  // typed, so loads, variables, constants, and arithmetic all reference type
  // ids.
  auto type_void(U32 result_id) -> void;
  auto type_bool(U32 result_id) -> void;
  auto type_int(U32 result_id, U32 width, Bool signedness) -> void;
  auto type_float(U32 result_id, U32 width) -> void;
  auto type_vector(U32 result_id, U32 component_type_id, U32 component_count)
      -> void;
  auto type_image(
      U32 result_id,
      U32 sampled_type_id,
      Dim dim,
      U32 depth,
      U32 arrayed,
      U32 multisampled,
      U32 sampled,
      ImageFormat format) -> void;
  auto type_sampler(U32 result_id) -> void;
  auto type_sampled_image(U32 result_id, U32 image_type_id) -> void;
  auto type_array(U32 result_id, U32 element_type_id, U32 length_id) -> void;
  auto type_struct(
      U32 result_id,
      Perimortem::Core::View::Vector<U32> member_type_ids) -> void;
  auto type_pointer(U32 result_id, StorageClass storage_class, U32 type_id)
      -> void;
  auto type_function(U32 result_id, U32 return_type_id) -> void;

  // Constants and variables create module scope ids. A variable's result type
  // is always a pointer type. Its storage class decides whether it is input,
  // output, push constant, uniform resource, or function local storage.
  auto constant(U32 result_type_id, U32 result_id, U32 value) -> void;
  auto constant_64(U32 result_type_id, U32 result_id, U64 value) -> void;
  auto constant_flag(U32 result_type_id, U32 result_id, Bool value) -> void;
  auto constant_composite(
      U32 result_type_id,
      U32 result_id,
      Perimortem::Core::View::Vector<U32> constituents) -> void;
  auto variable(U32 result_type_id, U32 result_id, StorageClass storage_class)
      -> void;

  // Body instructions are used inside a function after a label has opened a
  // basic block. Result producing instructions take both a result type id and a
  // fresh result id, matching SPIR V's SSA like value model.
  auto load(U32 result_type_id, U32 result_id, U32 pointer_id) -> void;
  auto store(U32 pointer_id, U32 object_id) -> void;
  auto access_chain(
      U32 result_type_id,
      U32 result_id,
      U32 base_id,
      Perimortem::Core::View::Vector<U32> index_ids) -> void;
  auto vector_shuffle(
      U32 result_type_id,
      U32 result_id,
      U32 vector_1_id,
      U32 vector_2_id,
      Perimortem::Core::View::Vector<U32> components) -> void;
  auto composite_construct(
      U32 result_type_id,
      U32 result_id,
      Perimortem::Core::View::Vector<U32> constituents) -> void;
  auto composite_extract(
      U32 result_type_id,
      U32 result_id,
      U32 composite_id,
      Perimortem::Core::View::Vector<U32> indexes) -> void;
  auto image_sample_implicit_lod(
      U32 result_type_id,
      U32 result_id,
      U32 sampled_image_id,
      U32 coordinate_id) -> void;
  auto fadd(U32 result_type_id, U32 result_id, U32 left_id, U32 right_id)
      -> void;
  auto fsub(U32 result_type_id, U32 result_id, U32 left_id, U32 right_id)
      -> void;
  auto fmul(U32 result_type_id, U32 result_id, U32 left_id, U32 right_id)
      -> void;
  auto fdiv(U32 result_type_id, U32 result_id, U32 left_id, U32 right_id)
      -> void;
  auto frem(U32 result_type_id, U32 result_id, U32 left_id, U32 right_id)
      -> void;
  auto convert_u_to_f(U32 result_type_id, U32 result_id, U32 value_id) -> void;
  auto fconvert(U32 result_type_id, U32 result_id, U32 value_id) -> void;

  // Functions contain one or more labelled basic blocks.
  auto function(
      U32 result_type_id,
      U32 result_id,
      FunctionControl control,
      U32 function_type_id) -> void;
  auto label(U32 result_id) -> void;
  auto return_void() -> void;
  auto function_end() -> void;

  // Counts the 32 bit words needed for a SPIR V literal string, including its
  // required NUL byte and padding.
  static auto literal_string_word_count(Perimortem::Core::View::Bytes text)
      -> Count;

  // Lightweight structural check used by tests and the shader compiler. It only
  // verifies the header and instruction bounds. It does not prove semantic
  // SPIR V validity.
  static auto is_valid_module(Perimortem::Core::View::Bytes words) -> Bool;

 private:
  Perimortem::Memory::Dynamic::Bytes& words;
};

}  // namespace Tetrodotoxin::Terminal::Spirv::Assembler
