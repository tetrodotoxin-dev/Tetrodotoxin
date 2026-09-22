// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/modules/import.hpp"

auto ttx_import_representation() -> const ttx_representation* {
  return &Ttx::Data::Form::Compiled<
      Ttx::Data::Form::Native<ttx_import>::reference>::get_representation();
}
