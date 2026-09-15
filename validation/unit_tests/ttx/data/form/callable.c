// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_tests/ttx/data/form/callable.h"

#include <stdarg.h>
#include <stddef.h>

static U32 sum(U32 a, U32 b, U32 c, U32 d) {
  return a + b + c + d;
}

static R64 variadic(U32 count, ...) {
  va_list arguments;
  va_start(arguments, count);

  R64 result = 0;
  for (U32 i = 0; i < count; ++i) {
    result += va_arg(arguments, R64);
  }

  va_end(arguments);
  return result;
}

static ttx_test_vector twice(ttx_test_vector value) {
  return value + value;
}

const ttx_schema* ttx_test_callable_schema(void) {
  static const ttx_schema integer = {
    4,
    4,
    TTX_SCHEMA_VALUE,
    {.value = {TTX_SCHEMA_U32, 0}},
  };
  static const ttx_schema real = {
    8,
    8,
    TTX_SCHEMA_VALUE,
    {.value = {TTX_SCHEMA_R64, 0}},
  };
  static const ttx_schema vector = {
    16,
    16,
    TTX_SCHEMA_VALUE,
    {.value = {TTX_SCHEMA_V128, 0}},
  };

  // Four independently authored argument entries must equal the C++ side's
  // one repeated entry. The variadic list describes only its named prefix.
  static const ttx_schema_argument four[] = {
    {{&integer, 0}, 1},
    {{&integer, 0}, 1},
    {{&integer, 0}, 1},
    {{&integer, 0}, 1},
  };
  static const ttx_schema_argument count[] = {{{&integer, 0}, 1}};
  static const ttx_schema_argument packed[] = {{{&vector, 0}, 1}};
  static const ttx_schema functions[] = {
    {
      8,
      8,
      TTX_SCHEMA_CALLABLE,
      {.callable = {four, 4, {&integer, 0}, TTX_SCHEMA_SYSTEM_V_AMD64}},
    },
    {
      8,
      8,
      TTX_SCHEMA_CALLABLE,
      {.callable = {count, 1, {&real, 0}, TTX_SCHEMA_SYSTEM_V_AMD64_VARIADIC}},
    },
    {
      8,
      8,
      TTX_SCHEMA_CALLABLE,
      {.callable = {packed, 1, {&vector, 0}, TTX_SCHEMA_SYSTEM_V_AMD64}},
    },
  };
  static const ttx_schema_position fields[] = {
    {{&functions[0], 0}, offsetof(ttx_test_callables, sum)},
    {{&functions[1], 0}, offsetof(ttx_test_callables, variadic)},
    {{&functions[2], 0}, offsetof(ttx_test_callables, twice)},
  };
  static const ttx_schema table = {
    sizeof(ttx_test_callables),
    _Alignof(ttx_test_callables),
    TTX_SCHEMA_COMPOSITE,
    {.composite = {fields, 3}},
  };

  return &table;
}

const void* ttx_test_callable_table(void) {
  static const ttx_test_callables table = {sum, variadic, twice};
  return &table;
}
