// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"
#include "validation/unit_tests/ttx/data/form/preparation.hpp"

#include "ttx/data/form/compiled.hpp"
#include "ttx/data/form/compiler.hpp"

using namespace Perimortem;
using namespace Ttx::Data;
using namespace Ttx::Data::Form;

static Validation::Harness Pointers = {.name = "TTX::Data::Form::Pointer"_view};

// Constant evaluation may build a temporary reference to itself. Only the
// finished bytes escape preparation, so no constexpr allocation or source
// pointer is retained in the publication.
static consteval auto recursive_bytes() -> Core::Static::Bytes<8> {
  auto node = Schema::composite({}, 8, 8);
  const auto pointer = Schema::pointer(&node);
  const Schema::Position field(pointer, 0);
  node = Schema::composite({&field, 1}, 8, 8);
  Compiler compiler;
  if (compiler.compile(node) != Status::Success || compiler.get_size() != 8) {
    return Core::Static::Bytes<8>();
  }

  Core::Static::Bytes<8> bytes;
  compiler.write(Core::Access::Bytes(bytes.get_data(), 8));
  return bytes;
}

static constexpr auto recursive = recursive_bytes();
static_assert(recursive[4] == 0x80 && recursive[5] == 0x22);

// One target is used both inline and indirectly. The pointer modifier belongs
// to the reference, so neither use changes or copies that target definition.
PERIMORTEM_UNIT_TEST(Pointers, reference_modifier) {
  Validation::DataTests::Preparation prepare;
  const auto integer = Schema::primitive(Schema::Value::U32);
  const Schema::Position member(integer, 0);
  const auto target = Schema::composite({&member, 1}, 4, 4);
  const Schema::Reference direct(target);
  const auto indirect = Schema::pointer(&target);
  EXPECT(direct.schema == indirect.schema);
  EXPECT_NOT(direct.is_pointer());
  EXPECT(indirect.is_pointer());
  const Schema::Position fields[] = {{direct, 0}, {indirect, 8}};
  const auto& form = prepare(Schema::composite({fields, 2}, 16, 8));
  const auto a =
      Encoding::Element::decode(form.get_bytes(), 1, form.get_depth());
  const auto b =
      Encoding::Element::decode(form.get_bytes(), 2, form.get_depth());
  EXPECT_EQ(a.type, b.type);
  EXPECT(a.is_inline());
  EXPECT(b.is_pointer());

  Compiler compiler;
  EXPECT(compiler.compile(Schema::Reference(&target, 2)) == Status::Invalid);
}

PERIMORTEM_UNIT_TEST(Pointers, opaque_spelling) {
  Validation::DataTests::Preparation prepare;
  const U8 expected[] = {0x11, 0x80, 0x80, 0, 0x80, 0x20, 0, 1};
  const auto& pointer = prepare(Schema::pointer());
  EXPECT(pointer.compatible(Representation(expected, sizeof(expected))));
  // The native observation name does not introduce a second wire code.
  EXPECT(
      pointer.compatible(prepare(Schema::primitive(Schema::Value::Pointer))));

  const auto integer = Schema::primitive(Schema::Value::U32);
  const auto& typed = prepare(Schema::pointer(&integer));
  EXPECT_NOT(pointer.compatible(typed));
  typed.next(0).visit(
      [&](Representation::Position value) {
        EXPECT(value.get_value() == Schema::Value::Pointer);
        EXPECT_EQ(value.get_extent(), Count(8));
      },
      [&](Status) { EXPECT(False); });
}

// Both allocations describe one recursive shape. Doubling the source graph
// must not double the emitted bodies or change their reference numbering.
PERIMORTEM_UNIT_TEST(Pointers, recursive_sharing) {
  Validation::DataTests::Preparation prepare;
  auto a = Schema::composite({}, 8, 8);
  auto b = Schema::composite({}, 8, 8);
  const auto pa = Schema::pointer(&a), pb = Schema::pointer(&b);
  const Schema::Position ab(pb, 0), ba(pa, 0);
  a = Schema::composite({&ab, 1}, 8, 8);
  b = Schema::composite({&ba, 1}, 8, 8);
  const auto& ready = prepare(a);
  EXPECT(ready.compatible(Representation(recursive.get_data(), 8)));

  Count count = 0;
  EXPECT(ready.visit([&](Representation::Position position) {
    ++count;
    EXPECT(position.get_value() == Schema::Value::Pointer);
    return Status::Success;
  }) == Status::Success);
  EXPECT_EQ(count, Count(1));

  // A difference beyond a recursive edge still rejects agreement. Structural
  // sharing must preserve the other fields even when a target repeats.
  const auto integer = Schema::primitive(Schema::Value::U64);
  const Schema::Position different[] = {{pa, 0}, {integer, 8}};
  b = Schema::composite({different, 2}, 16, 8);
  EXPECT_NOT(ready.compatible(prepare(a)));
}

PERIMORTEM_UNIT_TEST(Pointers, inline_cycle_rejected) {
  auto node = Schema::composite({}, 8, 8);
  const Schema::Position self(node, 0);
  node = Schema::composite({&self, 1}, 8, 8);
  Compiler compiler;
  EXPECT(compiler.compile(node) == Status::Invalid);
}

// The callable is a pointer edge even when its argument is its containing
// struct by value. Preparation must not mistake that signature for inline
// containment and must not omit a function with no payload arguments.
PERIMORTEM_UNIT_TEST(Pointers, recursive_callable) {
  Validation::DataTests::Preparation prepare;
  auto node = Schema::composite({}, 8, 8);
  const Schema::Argument self(node);
  const auto callback =
      Schema::callable(Schema::Convention::SystemVAMD64, {&self, 1});
  const Schema::Position field(callback, 0);
  node = Schema::composite({&field, 1}, 8, 8);
  const auto& ready = prepare(node);
  EXPECT_EQ(ready.get_bytes().get_size(), Count(16));
  EXPECT(ready.visit([](Representation::Position position) {
    return position.get_value() == Schema::Value::Pointer ? Status::Success
                                                          : Status::Invalid;
  }) == Status::Success);
}

// Equal independent pointees allow adjacent pointer occurrences to compact
// after references are reconciled. Explicitly spaced pointer slots keep D.
PERIMORTEM_UNIT_TEST(Pointers, pointer_run) {
  Validation::DataTests::Preparation prepare;
  const auto integer = Schema::primitive(Schema::Value::U32);
  const Schema::Position value(integer, 0);
  const auto a = Schema::composite({&value, 1}, 4, 4);
  const auto b = Schema::composite({&value, 1}, 4, 4);
  const auto pa = Schema::pointer(&a), pb = Schema::pointer(&b);
  const Schema::Position fields[] = {{pa, 0}, {pb, 16}};
  const auto& explicit_fields = prepare(Schema::composite({fields, 2}, 24, 8));
  const auto& ranged = prepare(Schema::range(pa, 2, 16, 24, 8));
  EXPECT(explicit_fields.compatible(ranged));
  EXPECT_EQ(ranged.get_bytes().get_size(), Count(16));
  const Count coordinate = 16;
  EXPECT(
      ranged.visit(
          Core::View::Vector<Count>(&coordinate, 1),
          [&](Representation::Position position) {
            EXPECT_EQ(position.offset, coordinate);
            return Status::Success;
          }) == Status::Success);
}

// A ring with one distinct field needs information to propagate around the
// entire cycle before equality settles. Independent allocation and reversed
// source member enumeration still have to produce exactly the same stream.
PERIMORTEM_UNIT_TEST(Pointers, distinct_ring) {
  Validation::DataTests::Preparation prepare;
  constexpr Count size = 32;
  Schema nodes[2][size];
  Schema::Reference pointers[2][size];
  Schema::Position fields[2][size][2];
  const auto integer = Schema::primitive(Schema::Value::U32);
  const auto real = Schema::primitive(Schema::Value::R32);
  for (Count copy = 0; copy < 2; ++copy) {
    for (Count i = 0; i < size; ++i) {
      pointers[copy][i] = Schema::pointer(&nodes[copy][(i + 1) % size]);
      fields[copy][i][copy] = Schema::Position(pointers[copy][i], 0);
      fields[copy][i][1 - copy] =
          Schema::Position(i + 1 == size ? real : integer, 8);
      nodes[copy][i] = Schema::composite({fields[copy][i], 2}, 16, 8);
    }
  }

  const auto& first = prepare(nodes[0][0]);
  EXPECT(first.compatible(prepare(nodes[1][0])));
  EXPECT_EQ(first.get_bytes().get_size(), size * 12);
}

static constexpr auto pointer = Schema::pointer();
static constexpr auto scalar = Schema::primitive(Schema::Value::U32);
static constexpr auto& narrow_pointer =
    Compiled<pointer, 4>::get_representation();
static constexpr auto& wide_pointer =
    Compiled<pointer, 8>::get_representation();

// Neither target identity nor the compiler's native process should affect a
// pointer free form. It remains the same agreement across both pointer widths.
static_assert(Compiled<scalar, 8>::get_representation().compatible(
    Compiled<scalar, 4>::get_representation()));

PERIMORTEM_UNIT_TEST(Pointers, pointer_width) {
  const U8 expected[] = {
    0x10, 0, 0, 0, 1, 0, 0, 0, 0x11, 0x40, 0x40, 0, 0x80, 0x10, 0, 1,
  };
  EXPECT(narrow_pointer.get_bytes() == Core::View::Bytes(expected));
  EXPECT(narrow_pointer.get_pointer_size() == 4);
  EXPECT_EQ(narrow_pointer.get_extent(), Count(4));
  EXPECT_EQ(narrow_pointer.get_alignment(), Count(4));
  EXPECT(!narrow_pointer.compatible(wide_pointer));

  Memory::Allocator::Arena arena;
  Representation::compile(pointer, arena, 4)
      .visit(
          [&](const Representation& form) {
            EXPECT(form.compatible(narrow_pointer));
          },
          [&](Status) { EXPECT(false); });

  narrow_pointer.next(0).visit(
      [&](Representation::Position position) {
        EXPECT_EQ(position.get_extent(), Count(4));
        EXPECT(position.get_value() == Schema::Value::Pointer);
      },
      [&](Status) { EXPECT(false); });

  const Count coordinates[] = {0};
  Count visits = 0;
  EXPECT(
      narrow_pointer.visit(coordinates, [&](Representation::Position position) {
        EXPECT_EQ(position.get_extent(), Count(4));
        ++visits;
        return Status::Success;
      }) == Status::Success);
  EXPECT_EQ(visits, Count(1));
}

// The C compilation entry receives a runtime width. Reject unsupported storage
// there as well as in the ordinary C++ runtime entry.
PERIMORTEM_UNIT_TEST(Pointers, unsupported_width) {
  Compiler compiler;
  EXPECT(compiler.compile(pointer, 3) == Status::Unsupported);

  Memory::Allocator::Arena arena;
  Representation::compile(pointer, arena, 3)
      .visit(
          [&](const Representation&) { EXPECT(false); },
          [&](Status status) { EXPECT(status == Status::Unsupported); });
}

PERIMORTEM_UNIT_TEST(Pointers, identical_geometry) {
  // Both arrays have the same starts, enclosing extent and alignment. The
  // pointer width still distinguishes four byte pointers with gaps from eight
  // byte pointers, even though their ordinary descriptor blocks are identical.
  static constexpr auto array = Schema::range(pointer, 2, 8, 16, 8);
  const auto& narrow = Compiled<array, 4>::get_representation();
  const auto& wide = Compiled<array, 8>::get_representation();
  EXPECT(narrow.get_blocks() == wide.get_blocks());
  EXPECT(!narrow.compatible(wide));
}

PERIMORTEM_UNIT_TEST(Pointers, convention_agreement) {
  const auto function =
      Schema::callable(Schema::Convention::EmscriptenWasm32, {}, scalar, 4);
  Compiler compiler;
  EXPECT(compiler.compile(function, 4) == Status::Success);
  auto wrong = function;
  wrong.data.callable.convention = TTX_SCHEMA_SYSTEM_V_AMD64;
  EXPECT(compiler.compile(wrong, 4) == Status::Invalid);
}

PERIMORTEM_UNIT_TEST(Pointers, composed_pointers) {
  Memory::Allocator::Arena arena;
  const Representation::Member members[] = {
    {narrow_pointer, 0},
    {narrow_pointer, 4},
  };
  Representation::compose(members, 8, 4, arena)
      .visit(
          [&](const Representation& form) {
            EXPECT(form.get_pointer_size() == 4);
            Count count = 0;
            EXPECT(form.visit([&](Representation::Position position) {
              EXPECT_EQ(position.get_extent(), Count(4));
              ++count;
              return Status::Success;
            }) == Status::Success);
            EXPECT_EQ(count, Count(2));
          },
          [&](Status) { EXPECT(false); });

  const Representation::Member mixed[] = {
    {narrow_pointer, 0},
    {wide_pointer, 8},
  };
  Representation::compose(mixed, 16, 8, arena)
      .visit(
          [&](const Representation&) { EXPECT(false); },
          [&](Status status) { EXPECT(status == Status::Incompatible); });
}
