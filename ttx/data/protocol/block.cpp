// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/data/protocol/block.hpp"

#include "ttx/data/form/compiled.hpp"

auto ttx_block_view_representation() -> const ttx_representation* {
  return &Ttx::Data::Form::Compiled<
      Ttx::Data::Form::Native<ttx_block_view>::reference>::get_representation();
}

auto ttx_block_access_representation() -> const ttx_representation* {
  return &Ttx::Data::Form::Compiled<
      Ttx::Data::Form::Native<ttx_block_access>::reference>::get_representation();
}
