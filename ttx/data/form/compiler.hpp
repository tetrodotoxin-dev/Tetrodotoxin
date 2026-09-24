// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/hash.hpp"

#include "perimortem/memory/const/vector.hpp"

#include "ttx/data/encoding/callable.hpp"
#include "ttx/data/encoding/struct.hpp"
#include "ttx/data/form/representation.h"
#include "ttx/data/form/schema.hpp"
#include "ttx/data/status.hpp"

namespace Ttx::Data::Form {

// The compiler is used to encode the canonical TTX format for transferring
// structured data descriptors. The wire format is a sequence of fixed width
// blocks, always encoded in little endian order. The whole buffer uses one
// width, but compilation promotes that width when any value needs more room.
// We do the normalization before publishing so consumers can compare bytes or
// follow offsets without reconstructing the source objects.
//
// Pointer storage can occupy four or eight bytes, aligned to that width.
// Compilation takes this width as a constexpr input, defaulting to the host.
// Compiled passes its template width through the same implementation.
// Calling convention remains part of each callable description because
// agreeing on pointer storage alone cannot establish how a function is called.
//
// Four byte pointer storage writes the little endian U64 value 0x100000010
// once, before the root. Its zero depth nibble distinguishes
// this prefix from a struct header. Bits four through 31 contain only the tag
// 0x10. The root follows at byte eight and every block reference remains
// relative to that root. Readers establish the width before interpreting
// pointer slots. Eight byte pointers need no prefix. Forms without reachable
// pointer descriptions also omit it, so ordinary scalar storage has identical
// bytes across these targets. The selected width applies to every reachable
// body. Composition rejects children with different pointer widths instead of
// translating addresses.
//
// Struct blocks describe the number of following element descriptors, the
// extent of the object including tail padding, and its required alignment.
// For a 32 bit block the fields, shown from most to least significant, are:
//
//   EEEEEEEE EEEEAAAA AAAACCCC CCCCFFFF
//
//   F: Encoding depth, in U32 chunks. One means 32 bits, two means 64,
//      three means 96, and so on through fifteen. Zero is invalid.
//   C: Number of direct element descriptor blocks following this header.
//      This counts compact descriptors, not their expanded repetitions.
//   A: Required alignment in bytes, expressed as a nonzero power of two.
//   E: Object extent in bytes, including padding after the last member.
//
// F always occupies bits zero through three, including in wider formats.
// Reading the first byte and masking with 0x0F establishes the block size as
// 4 * F bytes. The other nibble can contain part of C without making that
// bootstrap ambiguous. Every struct header repeats the same F because a
// reference needs one common block size throughout the publication.
//
// Element blocks describe occurrences of a primitive, struct or callable:
//
//   NNNNNNNN OOOOOOOO DDDDDDSC *PPPPPPP
//
//   N: Number of occurrences, always nonzero.
//   O: Byte offset of the first occurrence within its enclosing struct.
//   D: Distance in bytes between repeated starts. A singleton uses its width.
//   S: P references a struct header when set.
//   C: P references a callable header when set. S must be clear and * set.
//   *: The occupied value uses the buffer's pointer size and alignment.
//      Its target remains described by S, C and P but is not inline storage.
//   P: Primitive code or absolute header index, at byte P * 4 * F.
//
// S and C are mutually exclusive. S=0, C=0, *=1, P=0 is the canonical opaque
// pointer. Ordinary primitive code zero is invalid. Setting * changes the
// occupied width, not D, so separately spaced pointer slots remain expressible.
//
// Callable headers describe a realized signature rather than occupied bytes:
//
//   NNNNNNNN CCCCCCCC xxxxxxSC *PPPPPPP
//
//   N: Expanded number of formal arguments. For a variadic signature this is
//      its fixed prefix. Following argument blocks have counts summing to N.
//   C: Calling convention, 1 for System V AMD64 LP64, 2 for its variadic form,
//      3 for Emscripten Wasm32 and 4 for its variadic form.
//
//   x: Reserved, always zero. These bits participate in bytewise agreement.
//
//   S, C, *, P: Return description using the element selectors. Setting every
//      selector bit to one denotes void, only in this return slot.
//
// Argument blocks have O=0 and D=0. N repeats consecutive identical formal
// parameters, not an array parameter. No argument sorting takes place. A
// receiver is an explicit ordinary argument. Native arrays must be supplied
// through a pointer to their storage form. Nontrivial C++ objects likewise
// need a provider thunk with the declared C boundary. The variadic profile
// implies a trailing ellipsis whose concrete argument types belong to each
// call site. No signature interpretation or call is needed to transfer a table.
//
// After the optional pointer width prefix, the root is always a struct header,
// including for one callable pointer.
// Referenced callable headers inherit F from that root. Their return target
// is visited before their arguments when assigning first use block indices.
//
// Wider encodings use q = 8 * F. N and O occupy q bits, D occupies q minus two,
// the selectors occupy three bits, and P occupies q minus one. Callable ABI
// occupies O's q bits and its reserved field occupies D's q minus two bits.
// Struct C and A occupy q bits, F occupies four, and E occupies 16 * F minus
// four.
//
//   F   Block bits   Struct C/A/E bits   Element N/O/D/P bits
//   1       32             8/8/12                 8/8/6/7
//   2       64           16/16/28            16/16/14/15
//   3       96           24/24/44            24/24/22/23
//   4      128           32/32/60            32/32/30/31
//   8      256          64/64/124            64/64/62/63
//
// Numeric blocks are assembled with these shifts, where pointer_bit is q minus
// one:
//
//   struct   = F | (C << 4) | (A << (4 + q)) | (E << (4 + 2*q))
//   selectors = P | (* << pointer_bit) | (C << q) | (S << (q+1))
//   element  = selectors | (D << (q+2)) | (O << (2*q)) | (N << (3*q))
//   callable = return_selectors | (ABI << (2*q)) | (N << (3*q))
//
// All fields are unsigned. Wider profiles do not imply a native U96 or U128
// C++ type. Writers can emit each block as F / 2 U64 chunks followed by
// F % 2 U32 chunks. Start with the least significant bits of the block, so an
// odd depth leaves its most significant 32 bits for the final U32 operation:
//
//   F   Operations in stream order
//   1   U32
//   2   U64
//   3   U64, U32
//   4   U64, U64
//
// Perimortem::Core::Writer::Binary encodes each chunk in little endian order.
// Blocks remain consecutive, with no padding between them. After the last
// block, append four zero bytes only when needed to make the complete buffer
// a multiple of eight bytes. Those zeros participate in canonical comparison
// as buffer padding, leaving descriptor counts, block indices and F unchanged.
//
// Readers use U64 chunks at offsets 0, 8, 16 and so on from the whole buffer's
// start. Every such chunk is complete, even if it crosses a block boundary.
// A field spanning two chunks combines their relevant bits. Encoding::Block
// handles unaligned loads and converts the chunks to host byte order. Reading
// a field therefore needs neither a U32 tail read nor an assembled block.
// Unused high bits are zero, with no native struct padding, pointers or
// allocation addresses in the encoded buffer. Serialization does not allocate
// a separate object for each primitive or referenced body.
//
// Primitive codes use the Data vocabulary, independently of declaration order.
// The low six bits identify the primitive. Bit six specifies big endian
// payload storage when set, or little endian storage when clear. Higher P bits
// are zero for primitives, even in a wider profile. For a reference all P bits
// belong to the index instead. The descriptor words themselves remain little
// endian regardless of the payload order they describe.
//
//   Code   Primitive   Bytes
//     1       U8         1
//     2       U16        2
//     3       U32        4
//     4       U64        8
//     5       S8         1
//     6       S16        2
//     7       S32        4
//     8       S64        8
//     9       R32        4
//    10       R64        8
//    13       V64        8
//    14       V128      16
//    15       V256      32
//    16       V512      64
//
// Codes zero, eleven, twelve and other unassigned codes are reserved. Pointer
// is a native observation category, not a primitive code in this stream. U8
// and S8 clear the byte order bit because one byte has no ordering distinction.
// SIMD carriers have size and alignment equal to their listed width. Their
// codes preserve the vector ABI classification even when a scalar aggregate
// could occupy the same bytes. Lane operations belong to the semantic contract.
// Big endian vector storage reverses the complete carrier, not individual
// lanes.
//
// Complete canonical equality includes every reachable callable signature and
// ABI. A conforming provider's negotiated function pointers can therefore be
// called using that agreed signature. UUIDs still establish operation meaning,
// and owners still establish lifetime. Neither is inferred from payload bytes.
//
// A struct header is followed immediately by its C direct element blocks.
// Other bodies follow in first use depth first order. For example,
// after the root header and its descriptors come C1, C1's children C1C1 and
// C1C2, then C2 and its child C2C1. Each body includes its header and direct
// descriptors before any newly reached child body. References can therefore
// point forward or backward to an already emitted body. References aren't
// relative and are direct offsets from the root because readers establish
// their position from that root rather than resynchronizing within the stream.
//
// One stored body may serve many occurrences: each reference preserves its
// own offset, count and distance, and still denotes a struct boundary for every
// occurrence. Sharing the body never merges those occurrences into one object.
// References land only on matching header kinds. Inline containment is acyclic.
// Pointer targets and callable descriptions may refer back to an unfinished
// body because they add no inline storage. Numbering reserves a body's index
// before following those references and emits a reached body only once.
//
// Encoded buffers can be compared for equivalence by checking their lengths
// and comparing every byte. Equal canonical bytes establish the same data
// format, including its struct boundaries and geometry. They do not compare
// the runtime values being transported or establish a higher semantic type.
// To make equivalent schemas produce that one representation, compilation
// applies the following canonicalization rules before choosing the depth:
//
// 1. Keep struct boundaries, primitive identities, payload byte order,
//    offsets, extents and alignments. The root describes the whole object
//    directly. A struct reference is not replaced with its primitive children,
//    even when it contains only one member. Padding is described by offsets
//    and extents rather than invented members or synthetic structs.
//
// 2. Normalize storage occurrences in byte offset order. Signature arguments
//    retain declaration order. Invalid primitive codes, inline cycles,
//    impossible geometry and overlapping occupied placements are rejected.
//    Names and source enumeration order do not affect the resulting body.
//
// 3. Compact from the lowest offset. When the next occurrence has the same
//    normalized type, their start difference establishes a candidate distance.
//    Take the longest consecutive run of that type at that distance, then
//    continue with the first occurrence outside the run. Struct type equality
//    here includes the normalized child body, extent and alignment. Primitive
//    type equality includes its payload byte order.
//
// 4. Source range boundaries do not stop that compaction. Compare and consume
//    matching prefixes arithmetically, splitting a source run when necessary.
//    U32 starts 0, 4, 8, 16 normalize to a count of three at distance four and
//    one singleton at 16, even if the input grouped them as two pairs. Nested
//    repetition can be combined only while preserving the same starts and
//    struct occurrences. A count never moves through a struct reference into
//    its members. Scalar progressions and repeated struct bodies stay compact.
//    Repeated batches with gaps can require a separate run for each batch, so
//    their descriptor count grows with the discontinuities being described.
//
// 5. For N greater than one, D is the actual distance between starts. For N
//    equal to one, D is the intrinsic element size: primitive width or the
//    referenced header's E, or the buffer's pointer size. Argument D is zero.
//    Thus one U32 in an eight byte aligned, eight byte struct has element
//    distance four. An array of those structs has reference distance eight.
//    This leaves no discretionary padding value that could change the bytes of
//    a singleton.
//
// 6. Validate the final occupied end using the last start plus the element
//    size. N * D is not necessarily that end, because it includes a distance
//    after the last occurrence. Three U32s at distance eight end at byte 20.
//    A root extent of 24 preserves the remaining four bytes of tail padding.
//
// 7. Elide zero occurrences and empty members. With no values, the only result
//    is one root header with F one, C zero, A one and E zero, followed by four
//    zero padding bytes. A nonzero extent without values is invalid.
//    No unreachable empty body or zero N element is added to the buffer to
//    remember how that unit was authored.
//
// 8. Intern structurally equal normalized bodies, including bodies authored
//    independently. Acyclic targets settle in dependency order. Recursive
//    descriptions refine structural classes until no class splits, ignoring
//    graph allocation and run spelling, then merge newly equal runs. Equality
//    uses child structure rather than source pointers, hash values or names.
//    Assign their absolute block indices by first use depth first traversal
//    after compaction. Emit each distinct reachable body once, with no extra
//    body inventory or unreferenced records at the end.
//
// 9. Merge adjacent equal argument types by adding N, with O and D zero.
//    Their counts sum to the callable header N. Preserve ABI and return type.
//    The void sentinel fills the complete selector field at the chosen F.
//
// 10. Scan all normalized fields and assigned indices for the smallest F that
//    holds every value in its profile. One overflowing count, offset, distance,
//    reference, extent, alignment or descriptor count promotes the entire
//    buffer. A run is not split merely to avoid promotion. Struct headers
//    repeat the chosen F and callable headers inherit it from the root. Block
//    indices were assigned in blocks, so widening changes byte addresses
//    without changing traversal or reference numbering.
//
// 11. Write only that final depth, with unused bits and final padding zero.
//     Round the complete buffer size up to eight bytes. Hashing, runtime
//     versus constant evaluation, and allocation policy cannot affect the
//     output. Compiler owns temporary preparation and the caller owns the final
//     buffer, so construction storage disappears from the published descriptor.
//
// For example, U8 at offsets zero, one and two followed by U32 at offset four
// becomes one U8 descriptor with N three and D one, then one U32 descriptor
// with N one and D four. The header's E eight retains the surrounding padding.
// U32s at offsets zero, eight and sixteen use one descriptor with N three
// and D eight. Referencing three one member structs uses S one, while
// that flat primitive run uses S zero, so the two layouts cannot collide.
//
// Comparison is linear in the encoded byte length and requires no decoding.
// Access reads the depth from the root after any prefix, then follows counts,
// offsets and references. Repetition computes an instance start from O and D
// without storing an entry for each value. Enumeration necessarily visits each
// requested primitive occurrence, not merely each compressed descriptor.
//
class Compiler {
 public:
  constexpr Compiler() = default;
  Compiler(const Compiler&) = delete;
  constexpr Compiler(Compiler&&) = default;

  // Preparation owns one content inventory. Source graphs can disappear after
  // success because publication consumes only these normalized records.
  constexpr auto compile(
      Schema::Reference source,
      Count selected = sizeof(void*)) -> Status {
    clear();
    if (selected != 4 && selected != 8) {
      return Status::Unsupported;
    }

    pointer_size = selected;

    Count root = 0;
    if ((source.flags & ~TTX_SCHEMA_REFERENCE_POINTER) || !source.is_set()) {
      return Status::Invalid;
    }

    auto status = source.is_pointer() ? prepare_reference(source)
                                      : prepare(*source.schema, root);
    if (status != Status::Success) {
      return status;
    }

    if (source.is_pointer() || (source.schema && source.schema->get_kind() ==
                                                     Schema::Kind::Callable)) {
      const Count first = begin(
          source.get_extent(pointer_size), source.get_alignment(pointer_size));
      records.insert(describe(source));
      root = bodies.get_size();
      bodies.insert(Body());
      seal(first, root);
    }

    for (Count i = 0; status == Status::Success && i < bodies.get_size(); ++i) {
      if (bodies[i].size == Unseen) {
        Count body = 0;
        status = prepare(*bodies[i].schema, body);
      }
    }

    if (status != Status::Success) {
      return status;
    }

    return publish(root);
  }

  // Existing forms can be assembled without rebuilding their source Schemas.
  // Admission checks only the new placements. Imported bodies then use the
  // same interning and numbering as source compilation, including cycles.
  auto compose(
      Perimortem::Core::View::Vector<ttx_representation_member> members,
      Count extent,
      Count alignment) -> Status;

  constexpr auto get_size() const -> Count {
    return prefix_size() +
           Perimortem::Core::Data::align<8>(block_count * 4 * depth);
  }
  constexpr auto get_depth() const -> U8 { return depth; }

  // Preparation has settled indices and the common depth. Each content record
  // can therefore emit one block directly into the caller's buffer. get_size()
  // includes final padding, so one capacity check covers every write.
  constexpr auto write(Perimortem::Core::Access::Bytes target) const -> Status {
    if (target.get_size() < get_size()) {
      return Status::Bounds;
    }

    Writer writer(target);
    if (prefix_size()) {
      writer << U64(0x100000010);
    }
    for (Count item = first; item; item = bodies[item - 1].next) {
      const auto& body = bodies[item - 1];
      const auto& head = records[body.first];
      if (head.distance) {
        Encoding::Struct(body.size - 1, head.distance, head.offset)
            .encode(depth)
            .write(writer, depth);
      } else {
        const auto returned =
            published(Element(1, 0, 0, head.type, head.attributes & 7));
        Encoding::Callable(
            head.count, head.offset, returned, head.attributes & Void)
            .encode(depth)
            .write(writer, depth);
      }

      for (Count i = 1; i < body.size; ++i) {
        published(records[body.first + i]).encode(depth).write(writer, depth);
      }
    }

    if (writer.get_location() & 7) {
      writer << U32(0);
    }

    return Status::Success;
  }

 private:
  using Element = Encoding::Element;
  using Elements = Perimortem::Memory::Const::Vector<Element>;
  using Indices = Perimortem::Memory::Const::Vector<Count>;
  using View = Perimortem::Core::View::Vector<Element>;
  using Writer = Perimortem::Core::Writer::Binary<
      Perimortem::Core::Data::ByteOrder::Little>;
  static constexpr Count Unseen = Count(-1);
  static constexpr Count Active = Count(-2);

  // Preparation uses Void to mark a callable return header. Encoding converts
  // it to the reserved return selector so it never occupies an Element flag.
  static constexpr Count Void = 8;

  // Discovery keeps each source and its emitted span in one inventory. After
  // discovery, hashing uses the normalized span instead of the source address.
  // During settlement block holds Active or a canonical body ID. Numbering
  // replaces those temporary IDs with absolute output indices. Hash lookup
  // has then finished, allowing next to hold the emission order.
  struct Body {
    const Schema* schema = nullptr;
    Count first = 0;
    Count size = Unseen;
    U64 hash = 0;
    Count next = 0;
    Count block = Unseen;

    constexpr Body() = default;
    constexpr Body(const Schema* schema, U64 hash)
        : schema(schema), hash(hash) {}
    constexpr Body(Count first, Count size, U64 hash)
        : first(first), size(size), hash(hash) {}
  };
  struct Octets {
    U8 bytes[sizeof(Element)];
  };

  // Numbering already visits every emitted field. Accumulate their occupied
  // bits there so selecting the common depth needs no second descriptor walk.
  struct Limits {
    Count common = 0;
    Count reference = 0;
    Count distance = 0;
    Count extent = 0;
  };

  // Runtime composition remaps an admitted body's absolute block references
  // into this compiler's temporary inventory. These helpers share the same
  // private record ownership as source compilation, so normalization has one
  // implementation rather than a second externally mutable record API.
  auto import_body(
      const ttx_representation& form,
      Count block,
      Bool callable,
      Indices& indices) -> Count;

  constexpr auto clear() -> void {
    bodies.resize(0);
    records.resize(0);
    buckets.resize(0);
    first = 0;
    block_count = 0;
    depth = 0;
    has_pointers = False;
  }

  constexpr auto publish(Count root) -> Status {
    // A single body needs no interning. A struct and a callable are also
    // necessarily distinct, so a form containing just those has nothing to
    // merge. Larger graphs or two bodies of the same kind need structural
    // settlement.
    const Count count = bodies.get_size();
    if (count > 1 &&
        (count > 2 || bool(header(0).distance) == bool(header(1).distance))) {
      reconcile(root);
    }

    Limits limits;
    Count last = Unseen;
    first = root + 1;
    number(root, limits, last);
    return choose_depth(limits);
  }

  // The runtime view borrows the actual initialized content. Constant
  // evaluation cannot reinterpret an object pointer, so only that path copies
  // the same representation into byte storage for the existing byte hasher.
  static constexpr auto hash(View content) -> U64 {
    if consteval {
      Perimortem::Memory::Const::Vector<U8> bytes;
      bytes.resize(content.get_size() * sizeof(Element));
      for (Count i = 0; i < content.get_size(); ++i) {
        const auto word = __builtin_bit_cast(Octets, content[i]);
        for (Count j = 0; j < sizeof(Element); ++j) {
          bytes[i * sizeof(Element) + j] = word.bytes[j];
        }
      }

      return Perimortem::Core::Hash(
                 Perimortem::Core::View::Bytes(
                     bytes.get_data(), bytes.get_size()))
          .get_value();
    } else {
      return Perimortem::Core::Hash(
                 Perimortem::Core::View::Bytes(
                     reinterpret_cast<const U8*>(content.get_data()),
                     content.get_size() * sizeof(Element)))
          .get_value();
    }
  }

  static constexpr auto equal(View a, View b) -> Bool {
    return a.get_size() == b.get_size() &&
           Perimortem::Core::Data::compare(
               a.get_data(), b.get_data(), a.get_size());
  }

  static constexpr auto span(const Elements& data, Count first, Count size)
      -> View {
    return View(data.get_data() + first, size);
  }

  constexpr auto header(Count body) const -> const Element& {
    return records[bodies[body].first];
  }

  constexpr auto extent(Count body) const -> Count {
    const auto& value = header(body);
    return value.distance ? value.offset : pointer_size;
  }

  constexpr auto width(const Element& entry) const -> Count {
    return entry.is_pointer()  ? pointer_size
           : entry.is_inline() ? extent(entry.type)
                               : Schema::get_width(entry.get_value());
  }

  constexpr auto published(Element entry) const -> Element {
    if (entry.references()) {
      entry.type = bodies[entry.type].block;
    }

    return entry;
  }

  // Source memoization, body interning and recursive refinement all index
  // existing inventory entries. Sharing lookup and growth lets each phase use
  // those stored keys and hashes directly.
  static constexpr auto find(
      const auto& entries,
      const Indices& buckets,
      U64 fingerprint,
      auto matches) -> Count {
    if (buckets.is_empty()) {
      for (Count i = 0; i < entries.get_size(); ++i) {
        if (entries[i].hash == fingerprint && matches(i)) {
          return i;
        }
      }
    } else {
      for (Count item = buckets[fingerprint & (buckets.get_size() - 1)]; item;
           item = entries[item - 1].next) {
        if (entries[item - 1].hash == fingerprint && matches(item - 1)) {
          return item - 1;
        }
      }
    }

    return Unseen;
  }

  static constexpr auto place(auto& entries, Indices& buckets, Count id)
      -> void {
    auto& entry = entries[id];
    const Count bucket = entry.hash & (buckets.get_size() - 1);
    entry.next = buckets[bucket];
    buckets[bucket] = id + 1;
  }

  static constexpr auto reset(Indices& buckets, Count size) -> void {
    // Rounding table capacity to a power of two lets bucket lookup use a mask.
    // This capacity only affects scratch storage, leaving emitted order intact.
    if (size) {
      size = Count(1) << Perimortem::Core::Math::log2(size - 1);
    }

    buckets.resize(size);
    for (Count i = 0; i < size; ++i) {
      buckets[i] = 0;
    }
  }

  static constexpr auto index(auto& entries, Indices& buckets, Count id)
      -> void {
    // Small forms avoid index allocation. Larger inventories keep cached
    // hashes so growing a table does not rehash source pointers or bodies.
    if (entries.get_size() < 8) {
      return;
    }

    if (buckets.get_size() <= entries.get_size() * 2) {
      reset(buckets, entries.get_size() * 4);
      for (Count i = 0; i < entries.get_size(); ++i) {
        place(entries, buckets, i);
      }
    } else {
      place(entries, buckets, id);
    }
  }

  static constexpr auto source_hash(const Schema* schema) -> U64 {
    if consteval {
      // Constant evaluation can compare source pointers, but cannot hash
      // their addresses. Only source memoization uses this common bucket.
      return 0;
    } else {
      return Perimortem::Core::Hash(schema).get_value();
    }
  }

  constexpr auto find_source(const Schema* schema) const -> Count {
    return find(bodies, buckets, source_hash(schema), [&](Count i) {
      return bodies[i].schema == schema;
    });
  }

  constexpr auto source_slot(const Schema* schema) -> Count {
    const Count found = find_source(schema);
    if (found != Unseen) {
      return found;
    }

    const Count slot = bodies.get_size();
    bodies.insert(Body(schema, source_hash(schema)));
    if !consteval {
      index(bodies, buckets, slot);
    }

    return slot;
  }

  constexpr auto validate_value(const Schema& source) const -> Status {
    const Count size = Schema::get_width(source.get_value(), pointer_size);
    if (!size || source.get_extent() != size ||
        source.get_alignment() != size) {
      return Status::Invalid;
    }

    const auto order = source.get_byte_order();
    if (order != Schema::ByteOrder::Little && order != Schema::ByteOrder::Big) {
      return Status::Invalid;
    }

    return source.get_value() == Schema::Value::Pointer &&
                   order != Schema::ByteOrder::Little
               ? Status::Invalid
               : Status::Success;
  }

  constexpr auto prepare_reference(
      Schema::Reference reference,
      Bool argument = False) -> Status {
    if ((reference.flags & ~TTX_SCHEMA_REFERENCE_POINTER) ||
        !reference.is_set()) {
      return Status::Invalid;
    }

    const auto* source = reference.schema;
    if (!source) {
      return Status::Success;
    }

    if (argument && !reference.is_pointer() &&
        (source->get_kind() == Schema::Kind::Range || !source->get_extent())) {
      return Status::Invalid;
    }

    if (source->get_kind() == Schema::Kind::Value &&
        !(reference.is_pointer() &&
          source->get_value() == Schema::Value::Pointer)) {
      return validate_value(*source);
    }

    // Arguments describe a call boundary rather than inline containment.
    // Like pointer targets, their bodies can be completed after the caller.
    if (reference.is_pointer() || argument) {
      source_slot(source);
      return Status::Success;
    }

    Count body = 0;
    return prepare(*source, body);
  }

  // References already passed preparation. Describing one cannot append a
  // child body while its parent is emitting contiguous content.
  constexpr auto describe(Schema::Reference reference) -> Element {
    const auto* source = reference.schema;
    if (!source) {
      return Element(1, 0, pointer_size, 0, Element::Pointer);
    }

    if (source->get_kind() == Schema::Kind::Value &&
        !(reference.is_pointer() &&
          source->get_value() == Schema::Value::Pointer)) {
      const auto type = source->get_value();
      const Bool pointer =
          reference.is_pointer() || type == Schema::Value::Pointer;
      const Count size = pointer ? pointer_size : source->get_extent();
      const Count code =
          type == Schema::Value::Pointer
              ? 0
              : static_cast<U8>(type) |
                    ((source->get_extent() > 1 &&
                      source->get_byte_order() == Schema::ByteOrder::Big)
                         ? 64
                         : 0);
      return Element(1, 0, size, code, pointer ? Element::Pointer : 0);
    }

    const Count slot = find_source(source);
    const Bool callable = source->get_kind() == Schema::Kind::Callable;
    return Element(
        1, 0, reference.get_extent(pointer_size), slot,
        (callable ? Element::Callable | Element::Pointer : Element::Struct) |
            (reference.is_pointer() ? Element::Pointer : 0));
  }

  constexpr auto prepare(const Schema& source, Count& result) -> Status {
    const Count slot = source_slot(&source);
    if (bodies[slot].size == Active) {
      return Status::Invalid;
    }

    result = slot;
    if (bodies[slot].size != Unseen) {
      return Status::Success;
    }

    if (!source.get_alignment() ||
        (source.get_alignment() & (source.get_alignment() - 1))) {
      return Status::Invalid;
    }

    bodies[slot].size = Active;
    Status status = Status::Invalid;
    switch (source.get_kind()) {
    case Schema::Kind::Value:
      status = validate_value(source);
      if (status == Status::Success) {
        const Count first = begin(source.get_extent(), source.get_alignment());
        records.insert(describe(source));
        seal(first, result);
      }

      break;
    case Schema::Kind::Composite:
      status = composite(source, result);
      break;
    case Schema::Kind::Range:
      status = range(source, result);
      break;
    case Schema::Kind::Callable:
      status = callable(source, result);
      break;
    }

    return status;
  }

  constexpr auto begin(Count size, Count alignment) -> Count {
    const Count first = records.get_size();
    records.insert(Element(0, size, alignment));
    return first;
  }

  // One pending run serves arguments, ordinary fields and refinement keys.
  // Zero distance expresses argument repetition. Storage runs preserve actual
  // starts and may consume only a matching prefix of the next source run.
  struct Run {
    Element current;
    Count width = 0;
    Bool ordered = True;

    template <typename Emit>
    constexpr auto push(Element next, Count size, Emit emit) -> void {
      const Count last =
          current.offset +
          (current.count ? current.count - 1 : 0) * current.distance;
      const Bool follows = !current.count || next.offset >= last + width;
      ordered &= follows;
      if (follows && current.count && current.type == next.type &&
          current.attributes == next.attributes) {
        // The first pair establishes a distance. Later occurrences continue
        // the run when their start is one distance beyond its previous start.
        const Count distance = next.offset - last;
        if (current.count == 1 || distance == current.distance) {
          current.distance = distance;
          ++current.count;
          --next.count;
          if (!next.count) {
            return;
          }

          next.offset += next.distance;
          if (next.distance == distance) {
            current.count += next.count;
            return;
          }
        }
      }

      flush(emit);
      current = next;
      width = size;
    }

    template <typename Emit>
    constexpr auto flush(Emit emit) -> void {
      if (current.count) {
        if (current.count == 1) {
          current.distance = width;
        }

        emit(current);
        current.count = 0;
      }
    }
  };

  // Ranges contribute their existing runs and structs contribute a reference.
  // Feeding both into the pending run lets adjacent fields compact before
  // allocating their output records.
  constexpr auto append(
      Schema::Reference reference,
      Count offset,
      Count count,
      Count distance,
      Run& run) -> void {
    if (!count || !reference.get_extent(pointer_size)) {
      return;
    }

    const auto emit = [&](Element entry) { records.insert(entry); };
    if (!reference.is_pointer() &&
        reference.schema->get_kind() == Schema::Kind::Range) {
      const auto child = bodies[find_source(reference.schema)];
      const auto first = records[child.first + 1];
      const Bool joins =
          first.count == 1 || distance == first.count * first.distance;
      if (child.size == 2 && (count == 1 || joins)) {
        auto entry = first;
        entry.offset += offset;
        if (entry.count == 1 && count > 1) {
          entry.distance = distance;
        }

        entry.count *= count;
        run.push(entry, width(entry), emit);
      } else {
        for (Count repeat = 0; repeat < count; ++repeat) {
          for (Count i = 1; i < child.size; ++i) {
            auto entry = records[child.first + i];
            entry.offset += offset + repeat * distance;
            run.push(entry, width(entry), emit);
          }
        }
      }
    } else {
      auto entry = describe(reference);
      entry.offset = offset;
      entry.count = count;
      if (count > 1) {
        entry.distance = distance;
      }

      run.push(entry, width(entry), emit);
    }
  }

  constexpr auto composite(const Schema& source, Count& result) -> Status {
    const auto positions = source.get_positions();
    if (positions.get_size() && !positions.get_data()) {
      return Status::Invalid;
    }

    for (const auto position : positions) {
      const auto reference = position.get_reference();
      if (position.offset > source.get_extent() ||
          reference.get_extent(pointer_size) >
              source.get_extent() - position.offset) {
        return Status::Invalid;
      }

      const auto status = prepare_reference(reference);
      if (status != Status::Success) {
        return status;
      }
    }

    const Count first = begin(source.get_extent(), source.get_alignment());
    Run run;
    for (const auto position : positions) {
      append(position.get_reference(), position.offset, 1, 0, run);
    }

    return finish(first, run, result);
  }

  constexpr auto range(const Schema& source, Count& result) -> Status {
    const auto& value = source.get_range();
    const auto reference = value.get_element();
    const auto status = prepare_reference(reference);
    if (status != Status::Success) {
      return status;
    }

    const Count count = value.get_count();
    const Count size = reference.get_extent(pointer_size);
    const Count distance = value.get_distance();
    if ((!count || !size) && source.get_extent()) {
      return Status::Invalid;
    }

    if (size && count) {
      if (size > source.get_extent() || (count > 1 && distance < size)) {
        return Status::Invalid;
      }

      if (count > 1 && count - 1 > (source.get_extent() - size) / distance) {
        return count - 1 > (Count(-1) - size) / distance ? Status::Overflow
                                                         : Status::Invalid;
      }
    }

    const Count first = begin(source.get_extent(), source.get_alignment());
    Run run;
    append(reference, 0, count, distance, run);
    return finish(first, run, result);
  }

  constexpr auto callable(const Schema& source, Count& result) -> Status {
    if (source.get_extent() != pointer_size ||
        source.get_alignment() != pointer_size) {
      return Status::Invalid;
    }

    const auto& value = source.get_callable();
    if (value.get_result().flags & ~TTX_SCHEMA_REFERENCE_POINTER) {
      return Status::Invalid;
    }

    const auto convention = value.get_convention();
    const bool narrow = pointer_size == 4;
    const bool accepted =
        narrow ? convention == Schema::Convention::EmscriptenWasm32 ||
                     convention == Schema::Convention::EmscriptenWasm32Variadic
               : convention == Schema::Convention::SystemVAMD64 ||
                     convention == Schema::Convention::SystemVAMD64Variadic;
    if (!accepted) {
      return Status::Invalid;
    }

    const auto arguments = value.get_arguments();
    if (arguments.get_size() && !arguments.get_data()) {
      return Status::Invalid;
    }

    if (value.get_result().is_set()) {
      const auto status = prepare_reference(value.get_result(), True);
      if (status != Status::Success) {
        return status;
      }
    }

    Element head = value.get_result().is_set() ? describe(value.get_result())
                                               : Element(0, 0, 0, 0, Void);
    head.count = 0;
    head.offset = static_cast<U32>(convention);
    head.distance = 0;
    const Count first = records.get_size();
    records.insert(head);
    Run run;
    const auto emit = [&](Element entry) { records.insert(entry); };
    for (const auto& argument : arguments) {
      if (!argument.count) {
        return Status::Invalid;
      }

      if (argument.count > Count(-1) - records[first].count) {
        return Status::Overflow;
      }

      const auto status = prepare_reference(argument.get_reference(), True);
      if (status != Status::Success) {
        return status;
      }

      records[first].count += argument.count;
      auto entry = describe(argument.get_reference());
      entry.count = argument.count;
      entry.offset = entry.distance = 0;
      run.push(entry, 0, emit);
    }

    return finish(first, run, result);
  }

  static constexpr auto descend(Elements& heap, Count parent) -> void {
    for (Count child = parent * 2 + 1; child < heap.get_size();
         child = parent * 2 + 1) {
      if (child + 1 < heap.get_size() &&
          heap[child + 1].offset < heap[child].offset) {
        ++child;
      }

      if (heap[parent].offset <= heap[child].offset) {
        return;
      }

      Perimortem::Core::Data::swap(heap[parent], heap[child]);
      parent = child;
    }
  }

  // Discovery has already compacted ordered fields. Only a source whose runs
  // interleave needs this merge. Splitting at the next start preserves compact
  // ranges while exposing any actual overlap as an admission failure.
  constexpr auto reorder(Count first) -> Status {
    Elements heap;
    for (Count i = first + 1; i < records.get_size(); ++i) {
      heap.insert(records[i]);
    }

    for (Count i = heap.get_size() / 2; i; --i) {
      descend(heap, i - 1);
    }

    records.resize(first + 1);
    Run run;
    Count end = 0;
    const auto emit = [&](Element entry) { records.insert(entry); };
    while (!heap.is_empty()) {
      auto entry = heap[0];
      if (heap.get_size() > 1 && entry.count > 1) {
        const Count next =
            heap.get_size() > 2
                ? Perimortem::Core::Math::min(heap[1].offset, heap[2].offset)
                : heap[1].offset;
        const Count gap = next - entry.offset;
        const Count prefix = gap / entry.distance + (gap % entry.distance != 0);
        if (prefix && prefix < entry.count) {
          entry.count = prefix;
        }
      }

      // Advancing this run in place leaves its remainder in the heap. Restoring
      // heap order then selects the next start without reinserting that run.
      heap[0].count -= entry.count;
      if (heap[0].count) {
        heap[0].offset += entry.count * entry.distance;
      } else {
        heap[0] = heap[heap.get_size() - 1];
        heap.resize(heap.get_size() - 1);
      }

      descend(heap, 0);

      if (entry.offset < end) {
        return Status::Invalid;
      }

      end = entry.offset + (entry.count - 1) * entry.distance + width(entry);
      run.push(entry, width(entry), emit);
    }

    run.flush(emit);
    return Status::Success;
  }

  constexpr auto finish(Count first, Run& run, Count& result) -> Status {
    run.flush([&](Element entry) { records.insert(entry); });
    if (!run.ordered) {
      const auto status = reorder(first);
      if (status != Status::Success) {
        return status;
      }
    }

    if (records[first].distance && records.get_size() == first + 1) {
      if (records[first].offset) {
        return Status::Invalid;
      }

      records[first].distance = 1;
    }

    seal(first, result);
    return Status::Success;
  }

  constexpr auto seal(Count first, Count id) -> void {
    auto& body = bodies[id];
    body.first = first;
    body.size = records.get_size() - first;
    if (records[first].distance) {
      records[first].count = body.size - 1;
    }
  }

  // Direct settlement and cyclic refinement both share bodies by normalized
  // content. Hashing that content keeps sharing independent of source
  // addresses.
  static constexpr auto intern(
      Count id,
      Body body,
      const Elements& data,
      Perimortem::Memory::Const::Vector<Body>& entries,
      Indices& buckets) -> Count {
    entries[id] = body;
    const auto content = span(data, body.first, body.size);
    const Count found = find(entries, buckets, body.hash, [&](Count prior) {
      return entries[prior].block < Active &&
             equal(
                 content,
                 span(data, entries[prior].first, entries[prior].size));
    });
    const Count selected = found == Unseen ? id : found;
    entries[id].block = selected;
    if (found == Unseen && !buckets.is_empty()) {
      place(entries, buckets, id);
    }

    return selected;
  }

  // Settled reference identities can make adjacent runs equal. Acyclic bodies
  // compact within their own spans. Cyclic rounds write a separate inventory
  // so every body observes the same previous set of reference identities.
  constexpr auto project(Count id, auto canonical, Elements& target) const
      -> Body {
    const auto body = bodies[id];
    auto head = records[body.first];
    if (head.references()) {
      head.type = canonical(head.type);
    }

    const Count first = &target == &records ? body.first : target.get_size();
    Count output = first;
    const auto emit = [&](Element entry) {
      if (output == target.get_size()) {
        target.insert(entry);
      } else {
        target[output] = entry;
      }

      ++output;
    };
    emit(head);
    Run run;
    for (Count i = 1; i < body.size; ++i) {
      auto entry = records[body.first + i];
      const Count size = head.distance ? width(entry) : 0;
      if (entry.references()) {
        entry.type = canonical(entry.type);
      }

      run.push(entry, size, emit);
    }

    run.flush(emit);
    if (head.distance) {
      target[first].count = output - first - 1;
    }

    const Count size = output - first;
    return Body(first, size, hash(span(target, first, size)));
  }

  constexpr auto settle(Count id) -> Bool {
    if (bodies[id].block != Unseen) {
      return bodies[id].block != Active;
    }

    bodies[id].block = Active;
    const auto body = bodies[id];
    Bool changed = False;
    for (Count i = 0; i < body.size; ++i) {
      const auto entry = records[body.first + i];
      if (entry.references()) {
        if (!settle(entry.type)) {
          return False;
        }

        changed |= entry.type != bodies[entry.type].block;
      }
    }

    // Discovery already normalized these runs. Only a merged target identity
    // can change that result and require another compaction pass.
    Body key;
    if (changed) {
      key = project(
          id, [&](Count target) { return bodies[target].block; }, records);
    } else {
      key = Body(
          body.first, body.size, hash(span(records, body.first, body.size)));
    }

    intern(id, key, records, bodies, buckets);
    return True;
  }

  // Acyclic targets settle in place as the dependency walk returns. Only an
  // actual cycle needs a second content buffer: each round must observe the
  // previous round's complete classes, independent of traversal order.
  constexpr auto reconcile(Count& root) -> void {
    reset(buckets, bodies.get_size() < 8 ? 0 : bodies.get_size() * 2 + 1);
    if (settle(root)) {
      root = bodies[root].block;
      for (Count i = 0; i < bodies.get_size(); ++i) {
        bodies[i].block = Unseen;
      }

      return;
    }

    Indices classes, next;
    classes.resize(bodies.get_size());
    next.resize(bodies.get_size());
    Elements keys;
    Perimortem::Memory::Const::Vector<Body> candidates;
    candidates.resize(bodies.get_size());
    Bool changed;
    do {
      keys.resize(0);
      reset(buckets, bodies.get_size() * 2 + 1);
      changed = False;
      for (Count id = 0; id < bodies.get_size(); ++id) {
        const auto key =
            project(id, [&](Count target) { return classes[target]; }, keys);
        next[id] = intern(id, key, keys, candidates, buckets);
        changed |= next[id] != classes[id];
      }

      Perimortem::Core::Data::swap(classes, next);
    } while (changed);

    Perimortem::Core::Data::swap(records, keys);
    Perimortem::Core::Data::swap(bodies, candidates);
    for (Count i = 0; i < bodies.get_size(); ++i) {
      bodies[i].block = Unseen;
    }

    root = classes[root];
  }

  constexpr auto number(Count id, Limits& limits, Count& last) -> void {
    if (bodies[id].block != Unseen) {
      return;
    }

    const auto body = bodies[id];
    bodies[id].block = block_count;
    block_count += body.size;
    if (last != Unseen) {
      bodies[last].next = id + 1;
    }

    last = id;
    bodies[id].next = 0;
    const auto& head = records[body.first];
    limits.common |= head.count | (head.distance ? head.distance : head.offset);
    limits.extent |= head.distance ? head.offset : 0;
    for (Count i = 0; i < body.size; ++i) {
      const auto& entry = records[body.first + i];
      has_pointers |= entry.is_pointer();
      if (i) {
        limits.common |= entry.count | entry.offset;
        limits.distance |= entry.distance;
      }

      if (entry.references()) {
        number(entry.type, limits, last);
        limits.reference |= bodies[entry.type].block;
      } else {
        limits.reference |= entry.type;
      }
    }
  }

  constexpr auto choose_depth(const Limits& limits) -> Status {
    using Perimortem::Core::Math::log2;
    using Perimortem::Core::Math::max;
    Count required = max(Count(1), (log2(limits.common) + 7) / 8);
    required = max(required, (log2(limits.reference) + 8) / 8);
    required = max(required, (log2(limits.distance) + 9) / 8);
    required = max(required, (log2(limits.extent) + 19) / 16);
    if (required > 15 || block_count > (Count(-1) - 15) / (4 * required)) {
      return Status::Overflow;
    }

    depth = U8(required);
    return Status::Success;
  }

  Perimortem::Memory::Const::Vector<Body> bodies;
  Elements records;
  Indices buckets;
  Count first = 0;
  Count block_count = 0;
  U8 depth = 0;
  constexpr auto prefix_size() const -> Count {
    return has_pointers && pointer_size == 4 ? 8 : 0;
  }

  Count pointer_size = sizeof(void*);
  Bool has_pointers = False;
};

}  // namespace Ttx::Data::Form
