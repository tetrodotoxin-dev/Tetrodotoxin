// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/object.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem;

auto Core::Object<>::create(const Descriptor& descriptor) -> Object {
  return create(descriptor, 1, descriptor.get_size());
}

auto Core::Object<>::create(
    const Descriptor& descriptor,
    Count count,
    Count element_size) -> Object {
  Bool valid =
      descriptor.get_size() != 0 && descriptor.get_alignment() != 0 &&
      (descriptor.get_alignment() & (descriptor.get_alignment() - 1)) == 0 &&
      descriptor.get_alignment() <= Bibliotheca::allocation_alignment &&
      descriptor.get_finalizer() && count != 0 &&
      element_size == descriptor.get_size() &&
      count <= Count(-1) / element_size;
  if (!valid) {
    Diagnostics::Log::fatal(
        "Core Object received an invalid runtime descriptor."_view);
  }

  Bibliotheca::Allocation allocation =
      Bibliotheca::check_out(count * element_size);
  Bibliotheca::bind_object(allocation.ptr, &descriptor);
  return Object(allocation.ptr);
}

auto Core::Object<>::retain() const -> void {
  if (payload) {
    get_descriptor();
    Bibliotheca::reserve(payload);
  }
}

auto Core::Object<>::release() const -> void {
  if (!payload) {
    return;
  }

  const Descriptor& descriptor = get_descriptor();
  if (Bibliotheca::reservation_count(payload) == 1) {
    descriptor.get_finalizer()(payload);
  }

  Bibliotheca::remit(payload);
}

auto Core::Object<>::get_capacity() const -> Count {
  return Bibliotheca::capacity(payload);
}

auto Core::Object<>::get_reservations() const -> Count {
  return payload ? Bibliotheca::reservation_count(payload) : 0;
}

auto Core::Object<>::get_descriptor() const -> const Descriptor& {
  const void* descriptor = Bibliotheca::get_object(payload);
  if (!descriptor) {
    Diagnostics::Log::fatal(
        "Core Object received a handle without its runtime descriptor."_view);
  }

  return *static_cast<const Descriptor*>(descriptor);
}
