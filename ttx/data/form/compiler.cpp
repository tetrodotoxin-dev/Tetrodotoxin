// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/data/form/compiler.hpp"

#include "perimortem/memory/dynamic/map.hpp"

#include "ttx/data/form/representation.hpp"

using namespace Ttx::Data;
using namespace Ttx::Data::Form;

auto Compiler::import_body(
    const Representation& form,
    Count block,
    Bool callable,
    Indices& indices) -> Count {
  if (indices[block]) {
    return indices[block] - 1;
  }

  const auto bytes = form.get_blocks();
  const auto depth = form.get_depth();
  Count count = 0;
  Element head;
  if (callable) {
    const auto value = Encoding::Callable::decode(bytes, block, depth);
    head = Element(
        value.count, value.abi, 0, value.returns_void ? 0 : value.result.type,
        value.returns_void ? Void : value.result.attributes);
    for (Count arguments = 0; arguments < value.count; ++count) {
      arguments += Element::decode(bytes, block + count + 1, depth).count;
    }
  } else {
    const auto value = Encoding::Struct::decode(bytes, block, depth);
    count = value.count;
    head = Element(count, value.extent, value.alignment);
  }

  // Reserve the complete body before following references. A pointer back to
  // this body can then reuse its ID without reading an unfinished record or
  // expanding a recursive source graph.
  const Count id = bodies.get_size();
  const Count first = records.get_size();
  bodies.insert(Body(first, count + 1, 0));
  indices[block] = id + 1;
  records.resize(first + count + 1);
  for (Count i = 0; i <= count; ++i) {
    auto entry = i ? Element::decode(bytes, block + i, depth) : head;
    if (entry.is_pointer()) {
      if (has_pointers && pointer_size != form.get_pointer_size()) {
        return Unseen;
      }

      has_pointers = True;
      pointer_size = form.get_pointer_size();
    }

    if (entry.references()) {
      entry.type = import_body(
          form, entry.type, entry.attributes & Element::Callable, indices);
      if (entry.type == Unseen) {
        return Unseen;
      }
    }

    records[first + i] = entry;
  }

  return id;
}

auto Compiler::compose(
    Perimortem::Core::View::Vector<ttx_representation_member> members,
    Count size,
    Count alignment) -> Status {
  using namespace Perimortem::Memory;
  clear();
  if (!alignment || (alignment & (alignment - 1)) ||
      (members.get_size() && !members.get_data())) {
    return Status::Invalid;
  }

  Dynamic::Map<const U8*, Count> imported;
  Indices children;
  for (const auto member : members) {
    if (!member.representation) {
      return Status::Invalid;
    }

    const auto& form = *member.representation;
    if (member.offset > size || form.get_extent() > size - member.offset) {
      return Status::Invalid;
    }

    const Count child =
        imported.find(form.get_bytes().get_data())
            .visit(
                [&]() -> Count {
                  Indices indices;
                  indices.resize(
                      form.get_bytes().get_size() / (4 * form.get_depth()));
                  const Count id = import_body(form, 0, False, indices);
                  imported.insert(form.get_bytes().get_data(), id);
                  return id;
                },
                [](const auto& entry) { return entry.value; });
    if (child == Unseen) {
      return Status::Incompatible;
    }

    children.insert(child);
  }

  const Count first = begin(size, alignment);
  Count root = bodies.get_size();
  bodies.insert(Body());
  Run run;
  for (Count i = 0; i < members.get_size(); ++i) {
    const Count size = extent(children[i]);
    if (size) {
      run.push(
          Element(
              1, members.get_data()[i].offset, size, children[i],
              Element::Struct),
          size, [&](Element entry) { records.insert(entry); });
    }
  }

  const auto status = finish(first, run, root);
  return status == Status::Success ? publish(root) : status;
}

static auto publish(
    const Compiler& compiler,
    ttx_representation_allocator allocator,
    const ttx_representation** result) -> ttx_data_status {
  const Count size = compiler.get_size();
  if (size > Count(-1) - sizeof(Representation)) {
    return TTX_DATA_OVERFLOW;
  }

  auto* allocation = allocator.allocate(
      allocator.source, sizeof(Representation) + size, alignof(Representation));
  if (!allocation) {
    return TTX_DATA_BOUNDS;
  }

  auto* bytes = static_cast<U8*>(allocation) + sizeof(Representation);
  const auto written =
      compiler.write(Perimortem::Core::Access::Bytes(bytes, size));
  if (written != Status::Success) {
    return static_cast<ttx_data_status>(written);
  }

  *result = new (allocation, Perimortem::Core::Placement::Construct)
      Representation(bytes, size);
  return TTX_DATA_SUCCESS;
}

// The C entry prepares the description before asking its owner to allocate
// the exact output size. The view and bytes share that owner's lifetime, while
// Compiler destruction releases the temporary preparation storage.
auto ttx_representation_compile(
    ttx_schema_reference schema,
    Count pointer_size,
    ttx_representation_allocator allocator,
    const ttx_representation** result) -> ttx_data_status {
  if (!schema.is_set() || !allocator.allocate || !result) {
    return TTX_DATA_INVALID;
  }

  Compiler compiler;
  const auto status = compiler.compile(schema, pointer_size);
  if (status != Status::Success) {
    return static_cast<ttx_data_status>(status);
  }

  return publish(compiler, allocator, result);
}

auto ttx_representation_compose(
    const ttx_representation_member* members,
    Count count,
    Count extent,
    Count alignment,
    ttx_representation_allocator allocator,
    const ttx_representation** result) -> ttx_data_status {
  if (!allocator.allocate || !result) {
    return TTX_DATA_INVALID;
  }

  Compiler compiler;
  const auto status = compiler.compose(
      Perimortem::Core::View::Vector<ttx_representation_member>(members, count),
      extent, alignment);
  return status == Status::Success ? publish(compiler, allocator, result)
                                   : static_cast<ttx_data_status>(status);
}
