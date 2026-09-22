// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/abi/core/object.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem;

static auto select_object(U8* payload) -> Core::Object<> {
  return Core::Object<>(payload);
}

extern "C" auto perimortem_core_object_allocate(
    const Core::Object<>::Descriptor* descriptor) -> U8* {
  if (!descriptor) {
    Core::Diagnostics::Log::fatal(
        "Core Object ABI received an empty descriptor."_view);
  }

  return Core::Object<>::create(*descriptor).get_payload();
}

extern "C" auto perimortem_core_object_allocate_buffer(
    const Core::Object<>::Descriptor* descriptor,
    Count count,
    Count element_size) -> U8* {
  if (!descriptor) {
    Core::Diagnostics::Log::fatal(
        "Core Object ABI received an empty descriptor."_view);
  }

  return Core::Object<>::create(*descriptor, count, element_size).get_payload();
}

extern "C" auto perimortem_core_object_retain(U8* payload) -> void {
  select_object(payload).retain();
}

extern "C" auto perimortem_core_object_release(U8* payload) -> void {
  select_object(payload).release();
}

extern "C" auto perimortem_core_object_capacity(U8* payload) -> Count {
  return select_object(payload).get_capacity();
}

extern "C" auto perimortem_core_object_clone(
    U8* payload,
    const Core::Object<>::Descriptor* descriptor,
    Count element_size) -> U8* {
  if (!descriptor || element_size == 0) {
    Core::Diagnostics::Log::fatal(
        "Core Object clone received an invalid buffer contract."_view);
  }

  Core::Object<> current(payload);
  Count capacity = current.get_capacity();
  if (capacity == 0) {
    return {};
  }

  Count count = capacity / element_size;
  Core::Object<> replacement =
      Core::Object<>::create(*descriptor, count, element_size);
  Core::Data::copy(replacement.get_payload(), payload, count * element_size);
  current.release();
  return replacement.get_payload();
}

extern "C" auto perimortem_core_object_reservations(U8* payload) -> Count {
  return select_object(payload).get_reservations();
}

extern "C" auto perimortem_core_object_reserve(
    U8* payload,
    const Core::Object<>::Descriptor* descriptor,
    Count count,
    Count element_size,
    const U8* default_value) -> U8* {
  if (!descriptor || element_size == 0 || count > Count(-1) / element_size ||
      (count != 0 && !default_value)) {
    Core::Diagnostics::Log::fatal(
        "Core Object reserve received an invalid buffer contract."_view);
  }

  Core::Object<> current(payload);
  Count current_capacity = current.get_capacity() / element_size;
  if (count <= current_capacity) {
    return payload;
  }

  Count requested = Core::Math::max(count, current_capacity);
  if (requested == 0) {
    return {};
  }

  Core::Object<> replacement =
      Core::Object<>::create(*descriptor, requested, element_size);
  Count replacement_capacity = replacement.get_capacity() / element_size;
  if (current_capacity != 0) {
    Core::Data::copy(
        replacement.get_payload(), payload, current_capacity * element_size);
  }

  for (Count index = current_capacity; index < replacement_capacity; index++) {
    Core::Data::copy(
        replacement.get_payload() + index * element_size, default_value,
        element_size);
  }

  current.release();
  return replacement.get_payload();
}

extern "C" auto perimortem_core_object_finalize_trivial(U8*) -> void {}
