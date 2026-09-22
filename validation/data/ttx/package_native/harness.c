// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "Validation.Consumer/1.0/c_abi.h"

int main(void) {
  return TTX_FUNC_Validation_2eConsumer__Main__default_5fvalue_static() == 12 &&
                 TTX_FUNC_Validation_2eConsumer__Main__supplied_5fvalue_static() ==
                     15 &&
                 TTX_FUNC_Validation_2eConsumer__Main__static_5fvalue_static() ==
                     40
             ? 0
             : 1;
}
