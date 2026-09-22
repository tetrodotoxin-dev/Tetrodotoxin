// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/semantic/ownership/factory.hpp"

auto ttx_factory_representation() -> const ttx_representation* {
  return &Ttx::Data::Form::Compiled<
      Ttx::Data::Form::Native<ttx_factory>::reference>::get_representation();
}
