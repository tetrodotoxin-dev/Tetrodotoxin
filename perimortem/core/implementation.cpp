// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/implementation.hpp"

using namespace Perimortem;

Core::Implementation::Implementation(const Implementation& source)
    : object(source.object), projection(source.projection) {
  object.retain();
}

Core::Implementation::Implementation(Implementation&& source)
    : object(source.object), projection(source.projection) {
  source.object = Object<>();
  source.projection = nullptr;
}

Core::Implementation::~Implementation() {
  object.release();
}

auto Core::Implementation::operator=(const Implementation& source)
    -> Implementation& {
  if (object.get_payload() == source.object.get_payload() &&
      projection == source.projection) {
    return *this;
  }

  source.object.retain();
  object.release();
  object = source.object;
  projection = source.projection;
  return *this;
}

auto Core::Implementation::operator=(Implementation&& source)
    -> Implementation& {
  if (this == &source) {
    return *this;
  }

  object.release();
  object = source.object;
  projection = source.projection;
  source.object = Object<>();
  source.projection = nullptr;
  return *this;
}

auto Core::Implementation::retain(Object<> object, const void* projection)
    -> Option<Implementation> {
  BAIL_IF(object.is_empty() || projection == nullptr);
  object.retain();
  return Implementation(object, projection);
}
