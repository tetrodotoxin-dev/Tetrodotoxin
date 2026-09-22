// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;

Dynamic::Bytes::Bytes(Count reserved_capacity) : data(reserved_capacity) {}

Dynamic::Bytes::Bytes(const Core::View::Bytes view)
    : data(view.get_size()), size(view.get_size()) {
  if (!view.is_empty()) {
    Data::copy(data.get_access().get_data(), view.get_data(), view.get_size());
  }
}

Dynamic::Bytes::Bytes(const Bytes& rhs) : data(rhs.data), size(rhs.size) {}

Dynamic::Bytes::Bytes(Bytes&& rhs)
    : data(static_cast<Core::Object<U8>&&>(rhs.data)), size(rhs.size) {
  rhs.size = 0;
}

auto Dynamic::Bytes::operator=(Core::View::Bytes view) -> Bytes& {
  proxy(view);
  return *this;
}

auto Dynamic::Bytes::operator=(const Bytes& rhs) -> Bytes& {
  if (this == &rhs) {
    return *this;
  }

  data = rhs.data;
  size = rhs.size;
  return *this;
}

auto Dynamic::Bytes::operator=(Bytes&& rhs) -> Bytes& {
  if (this == &rhs) {
    return *this;
  }

  data = static_cast<Core::Object<U8>&&>(rhs.data);
  size = rhs.size;
  rhs.size = 0;
  return *this;
}

auto Dynamic::Bytes::append(U8 byte) -> void {
  Count size = get_size();
  auto access = prepare_write(size + 1);
  access.get_data()[size] = byte;
  this->size = size + 1;
}

auto Dynamic::Bytes::append(U8 byte, Count amount) -> void {
  if (amount == 0) {
    return;
  }

  Count size = get_size();
  auto access = prepare_write(size + amount);
  Data::set(access.get_data() + size, byte, amount);
  this->size = size + amount;
}

auto Dynamic::Bytes::concat(Core::View::Bytes view) -> void {
  if (view.is_empty()) {
    return;
  }

  if (view.get_size() > Count(-1) - get_size()) {
    Diagnostics::Log::fatal(
        "Dynamic Bytes concatenation exceeds the addressable size."_view);
  }

  Count size = get_size();
  Count required_size = size + view.get_size();
  auto access = prepare_write(required_size);
  Data::copy(access.get_data() + size, view.get_data(), view.get_size());
  this->size = required_size;
}

auto Dynamic::Bytes::get_access() -> Core::Access::Bytes {
  return prepare_write(get_size());
}

auto Dynamic::Bytes::prepare_write(Count required_capacity)
    -> Core::Access::Bytes {
  data.reserve(required_capacity);
  if (data.is_shared()) {
    data.clone();
  }

  return Core::Access::Bytes(data.get_access().get_data(), get_size());
}

auto Dynamic::Bytes::proxy(Core::View::Bytes view) -> void {
  Bytes replacement(view);
  *this = static_cast<Bytes&&>(replacement);
}

auto Dynamic::Bytes::set(U8 target) -> void {
  if (is_empty()) {
    return;
  }

  auto access = prepare_write(get_size());
  Data::set(access.get_data(), target, get_size());
}

auto Dynamic::Bytes::convert(U8 source, U8 target) -> void {
  auto access = prepare_write(get_size());
  for (Count index = 0; index < get_size(); index++) {
    if (access.get_data()[index] == source) {
      access.get_data()[index] = target;
    }
  }
}

auto Dynamic::Bytes::slice(Count start, Count size) const -> Core::View::Bytes {
  return get_view().slice(start, size);
}

auto Dynamic::Bytes::resize(Count new_size) -> void {
  ensure_capacity(new_size);
  size = new_size;
}

auto Dynamic::Bytes::forgetful_resize(Count required_size) -> void {
  Count capacity = get_capacity();
  Bool reusable = required_size <= capacity && required_size > (capacity >> 1);
  if (reusable && !data.is_shared()) {
    size = required_size;
    return;
  }

  data = Core::Object<U8>(required_size);
  size = required_size;
}

auto Dynamic::Bytes::shrink(Count bytes_to_remove) -> void {
  Count size = get_size();
  if (bytes_to_remove >= size) {
    clear();
    return;
  }

  if (bytes_to_remove == 0) {
    return;
  }

  auto access = prepare_write(size);
  Count remaining = size - bytes_to_remove;
  memmove(access.get_data(), access.get_data() + bytes_to_remove, remaining);
  this->size = remaining;
}

auto Dynamic::Bytes::operator[](Count index) const -> U8 {
  return get_view()[index];
}

auto Dynamic::Bytes::at(Count index) const -> U8 {
  return get_view()[index];
}

auto Dynamic::Bytes::clear() -> void {
  size = 0;
}

auto Dynamic::Bytes::reset() -> void {
  data = Core::Object<U8>();
  size = 0;
}

auto Dynamic::Bytes::ensure_capacity(Count required_size) -> void {
  if (required_size > get_capacity()) {
    data.reserve(required_size);
  }
}
