// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/data/protocol/direct.hpp"

#include "ttx/data/form/compiled.hpp"

auto ttx_direct_view_representation() -> const ttx_representation* {
  return &Ttx::Data::Form::Compiled<
      Ttx::Data::Form::Native<ttx_direct_view>::reference>::get_representation();
}

auto ttx_direct_access_representation() -> const ttx_representation* {
  return &Ttx::Data::Form::Compiled<
      Ttx::Data::Form::Native<ttx_direct_access>::reference>::get_representation();
}
