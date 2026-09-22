// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_DATA_FORM_SCHEMA_H
#define TTX_DATA_FORM_SCHEMA_H

#include "ttx/data/status.h"

// Right now native pointers are all 64 bit on the supported platforms. We'll
// need to extend this in the future (most likely at the ABI layer) but for now
// pointer schemas retain a description of their target while storage and
// payload traversal treat the pointer as 8 bytes axiomatically. Callable
// targets include the pointer depth as part of their call ABI so adding 32 bit
// pointer support in the future won't break back compat as it would be an ABI
// change which would prevent any successful negotation between 32 bit and 64
// bit systems.
//
// This check makes sure that we build that support BEFORE we deploy the C++
// runtime to a 32 bit system.
#ifdef __cplusplus
static_assert(sizeof(void*) == 8 && alignof(void*) == 8);
static_assert(sizeof(void (*)(void)) == 8 && alignof(void (*)(void)) == 8);
#else
_Static_assert(
    sizeof(void*) == 8 && _Alignof(void*) == 8,
    "TTX requires eight byte native pointers and alignment.");
_Static_assert(
    sizeof(void (*)(void)) == 8 && _Alignof(void (*)(void)) == 8,
    "TTX requires eight byte native function pointers and alignment.");
#endif

#ifdef __cplusplus
#include "perimortem/core/view/vector.hpp"
#endif

#define TTX_SCHEMA_VALUE ((U8)1)
#define TTX_SCHEMA_COMPOSITE ((U8)2)
#define TTX_SCHEMA_RANGE ((U8)4)
#define TTX_SCHEMA_CALLABLE ((U8)5)

typedef U8 ttx_schema_value;
#define TTX_SCHEMA_U8 ((ttx_schema_value)1)
#define TTX_SCHEMA_U16 ((ttx_schema_value)2)
#define TTX_SCHEMA_U32 ((ttx_schema_value)3)
#define TTX_SCHEMA_U64 ((ttx_schema_value)4)
#define TTX_SCHEMA_S8 ((ttx_schema_value)5)
#define TTX_SCHEMA_S16 ((ttx_schema_value)6)
#define TTX_SCHEMA_S32 ((ttx_schema_value)7)
#define TTX_SCHEMA_S64 ((ttx_schema_value)8)
#define TTX_SCHEMA_R32 ((ttx_schema_value)9)
#define TTX_SCHEMA_R64 ((ttx_schema_value)10)
#define TTX_SCHEMA_POINTER ((ttx_schema_value)12)
#define TTX_SCHEMA_V64 ((ttx_schema_value)13)
#define TTX_SCHEMA_V128 ((ttx_schema_value)14)
#define TTX_SCHEMA_V256 ((ttx_schema_value)15)
#define TTX_SCHEMA_V512 ((ttx_schema_value)16)
#define TTX_SCHEMA_LITTLE_ENDIAN ((U8)0)
#define TTX_SCHEMA_BIG_ENDIAN ((U8)1)

// Vector observations use fixed, aligned bit carriers. Returning these through
// an output pointer keeps Fragment usable without compiling its bootstrap for
// AVX or AVX512. Native callables use the corresponding SIMD register ABI.
// Lane interpretation belongs to the operation using those bits. Big endian
// storage reverses the whole carrier, just as for another primitive bit value.
#ifdef __cplusplus
#define TTX_VECTOR_ALIGNMENT(size) alignas(size)
#else
#define TTX_VECTOR_ALIGNMENT(size) _Alignas(size)
#endif
typedef struct ttx_vector64 {
  TTX_VECTOR_ALIGNMENT(8) U8 bytes[8];
} ttx_vector64;

typedef struct ttx_vector128 {
  TTX_VECTOR_ALIGNMENT(16) U8 bytes[16];
} ttx_vector128;

typedef struct ttx_vector256 {
  TTX_VECTOR_ALIGNMENT(32) U8 bytes[32];
} ttx_vector256;

typedef struct ttx_vector512 {
  TTX_VECTOR_ALIGNMENT(64) U8 bytes[64];
} ttx_vector512;
#undef TTX_VECTOR_ALIGNMENT

struct ttx_schema;

// A reference chooses whether a described target is stored inline or through
// a pointer. Keeping that choice on the edge lets one struct or callable
// description be reused without manufacturing a second Schema object.
#define TTX_SCHEMA_REFERENCE_POINTER ((U8)1)
typedef struct ttx_schema_reference {
  const struct ttx_schema* schema;
  U8 flags;
#ifdef __cplusplus
  constexpr ttx_schema_reference(
      const ttx_schema* schema = nullptr, U8 flags = 0)
      : schema(schema), flags(flags) {}

  constexpr ttx_schema_reference(const ttx_schema& schema)
      : schema(&schema), flags(0) {}

  constexpr auto is_pointer() const -> Bool {
    return flags & TTX_SCHEMA_REFERENCE_POINTER;
  }

  constexpr auto is_set() const -> Bool { return schema || is_pointer(); }
  constexpr auto get_extent() const -> Count;
  constexpr auto get_alignment() const -> Count;
#endif
} ttx_schema_reference;

typedef U32 ttx_schema_abi;
#define TTX_SCHEMA_SYSTEM_V_AMD64 ((ttx_schema_abi)1)
#define TTX_SCHEMA_SYSTEM_V_AMD64_VARIADIC ((ttx_schema_abi)2)

// An argument entry can describe consecutive identical formal parameters with
// one count. Their declaration order is retained, and compilation combines
// adjacent equal entries without turning those parameters into an array.
// A variadic signature describes only its fixed prefix through this list.
typedef struct ttx_schema_argument {
  ttx_schema_reference reference;
  Count count;
#ifdef __cplusplus
  constexpr ttx_schema_argument() : reference(), count(0) {}
  constexpr ttx_schema_argument(ttx_schema_reference reference, Count count = 1)
      : reference(reference), count(count) {}

  constexpr auto get_reference() const -> ttx_schema_reference {
    return reference;
  }
  constexpr auto get_count() const -> Count { return count; }
#endif
} ttx_schema_argument;

// An unset result reference denotes void. The actual receiver is an ordinary
// explicit argument. These facts describe the realized C call boundary and
// impose no native ownership or language object model on its provider.
typedef struct ttx_schema_callable {
  const ttx_schema_argument* arguments;
  Count count;
  ttx_schema_reference result;
  ttx_schema_abi abi;
#ifdef __cplusplus
  constexpr auto get_arguments() const
      -> Perimortem::Core::View::Vector<ttx_schema_argument> {
    return Perimortem::Core::View::Vector<ttx_schema_argument>(
        arguments, count);
  }
  constexpr auto get_result() const -> ttx_schema_reference { return result; }
  constexpr auto get_convention() const;
#endif
} ttx_schema_callable;

// Red and green can use the same U32 schema but occupy different positions in
// a color. Their placement belongs to the parent's entries, which lets a child
// description be reused without changing its meaning. Each placement supplies
// the byte offset from the intended wire record.
typedef struct ttx_schema_position {
  ttx_schema_reference reference;
  Count offset;
#ifdef __cplusplus
  constexpr ttx_schema_position(
      ttx_schema_reference reference = ttx_schema_reference(), Count offset = 0)
      : reference(reference), offset(offset) {}

  constexpr auto get_reference() const -> ttx_schema_reference {
    return reference;
  }
  constexpr auto get_offset() const -> Count { return offset; }
#endif
} ttx_schema_position;

typedef struct ttx_schema_composite {
  const ttx_schema_position* positions;
  Count count;
} ttx_schema_composite;

typedef struct ttx_schema_range {
  ttx_schema_reference element;
  Count count;
  Count distance;
#ifdef __cplusplus
  constexpr auto get_element() const -> ttx_schema_reference { return element; }

  constexpr auto get_count() const -> Count { return count; }

  constexpr auto get_distance() const -> Count { return distance; }
#endif
} ttx_schema_range;

typedef struct ttx_schema_primitive {
  U8 type;
  U8 byte_order;
} ttx_schema_primitive;

// Providers need to describe their intended wire format without deciding how
// every consumer will navigate it. Schema supplies that source vocabulary,
// including explicit offsets, repetition distances and padding. Compilation
// establishes the concrete geometry that transports borrow through a
// Representation. The source can then be discarded independently of that
// result.
//
// Composite preserves a real object boundary around its physical elements.
// Range can then be used to describe repeated geometry compactly, so compiling
// a million identical scalar positions can be processed in a single command.
//
// Names and the choice of which source supplies an output belong to the
// semantic layer. Source descriptions may be runtime objects or C++ constants.
// They need only remain stable during compilation. The resulting publication
// has its own lifetime and contains no source identity used to justify
// compatibility.
typedef struct ttx_schema {
  Count extent;
  Count alignment;
  U8 kind;
  union {
#ifdef __cplusplus
    ttx_schema_primitive value = {0, 0};
#else
    ttx_schema_primitive value;
#endif
    ttx_schema_composite composite;
    ttx_schema_range range;
    ttx_schema_callable callable;
  } data;

#ifdef __cplusplus
  // Kind selects the target's description. Reference flags independently
  // choose pointer storage, so a struct definition can serve both uses.
  enum class Kind : U8 {
    Value = TTX_SCHEMA_VALUE,
    Composite = TTX_SCHEMA_COMPOSITE,
    Range = TTX_SCHEMA_RANGE,
    Callable = TTX_SCHEMA_CALLABLE,
  };

  enum class Value : U8 {
    U8 = TTX_SCHEMA_U8,
    U16 = TTX_SCHEMA_U16,
    U32 = TTX_SCHEMA_U32,
    U64 = TTX_SCHEMA_U64,
    S8 = TTX_SCHEMA_S8,
    S16 = TTX_SCHEMA_S16,
    S32 = TTX_SCHEMA_S32,
    S64 = TTX_SCHEMA_S64,
    R32 = TTX_SCHEMA_R32,
    R64 = TTX_SCHEMA_R64,
    Pointer = TTX_SCHEMA_POINTER,
    V64 = TTX_SCHEMA_V64,
    V128 = TTX_SCHEMA_V128,
    V256 = TTX_SCHEMA_V256,
    V512 = TTX_SCHEMA_V512,
  };

  enum class Abi : U32 {
    SystemVAMD64 = TTX_SCHEMA_SYSTEM_V_AMD64,
    SystemVAMD64Variadic = TTX_SCHEMA_SYSTEM_V_AMD64_VARIADIC,
  };

  enum class ByteOrder : U8 {
    Little = TTX_SCHEMA_LITTLE_ENDIAN,
    Big = TTX_SCHEMA_BIG_ENDIAN
  };

  using Reference = ttx_schema_reference;
  using Position = ttx_schema_position;
  using Composite = ttx_schema_composite;
  using Range = ttx_schema_range;
  using Argument = ttx_schema_argument;
  using Callable = ttx_schema_callable;
  using V64 = ttx_vector64;
  using V128 = ttx_vector128;
  using V256 = ttx_vector256;
  using V512 = ttx_vector512;

  static constexpr auto get_width(Value type) -> Count;

  static constexpr auto primitive(
      Value type,
      ByteOrder order = ByteOrder::Little) -> ttx_schema;
  static constexpr auto pointer(const ttx_schema* target = nullptr) -> Reference;
  static constexpr auto callable(
      Abi abi,
      Perimortem::Core::View::Vector<Argument> arguments,
      Reference result = Reference()) -> ttx_schema;
  static constexpr auto composite(
      Perimortem::Core::View::Vector<Position> positions,
      Count extent,
      Count alignment = 1) -> ttx_schema;
  static constexpr auto range(
      Reference element,
      Count repeats,
      Count distance,
      Count extent,
      Count alignment = 1) -> ttx_schema;

  constexpr auto get_extent() const -> Count { return extent; }

  constexpr auto get_alignment() const -> Count { return alignment; }

  constexpr auto get_kind() const -> Kind { return static_cast<Kind>(kind); }

  // Selecting the kind establishes which part of the description we can
  // observe. These accessors preserve the source facts for compilation to
  // validate, including missing children and malformed position arrays.
  constexpr auto get_value() const -> Value {
    return static_cast<Value>(data.value.type);
  }

  constexpr auto get_byte_order() const -> ByteOrder {
    return static_cast<ByteOrder>(data.value.byte_order);
  }

  constexpr auto get_positions() const
      -> Perimortem::Core::View::Vector<Position> {
    return {data.composite.positions, data.composite.count};
  }

  constexpr auto get_range() const -> const Range& { return data.range; }

  constexpr auto get_callable() const -> const Callable& {
    return data.callable;
  }

  constexpr auto get_abi() const -> const ttx_schema& { return *this; }

#endif
} ttx_schema;

#endif
