// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/uuid.hpp"

#include "validation/benchmark.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/system/random.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Validation;

static constexpr Count uuid_batch = 1024;
static constexpr Uuid serialize_source_a(
    0x550e8400e29b41d4ULL,
    0xa716446655440000ULL);
static constexpr Uuid serialize_source_b(
    0x123e4567e89b42d3ULL,
    0xa456426614174000ULL);
static constexpr auto dashed_source_a =
    "550e8400-e29b-41d4-a716-446655440000"_bytes;
static constexpr auto dashed_source_b =
    "123e4567-e89b-42d3-a456-426614174000"_bytes;
static constexpr auto packed_source_a =
    "550e8400e29b41d4a716446655440000"_bytes;
static constexpr auto packed_source_b =
    "123e4567e89b42d3a456426614174000"_bytes;
static U64 uuid_source_offset = 0;

static auto select_uuid_source() -> void {
  uuid_source_offset = Random::generate();
}

static Harness SystemUuid = {
  .name = "UUIDs"_view,
  .setup = select_uuid_source,
};

PERIMORTEM_BENCHMARK(SystemUuid, generate_v4_x1024) {
  // Full UUID generation: entropy read + Philox seeding + 128 bit construction.
  Count value = 0;
  for (Count i = 0; i < uuid_batch; i++) {
    auto uuid = Uuid::generate_v4();
    value ^= uuid.get_value()[0];
  }

  Benchmark::prevent_optimization(value);
}

PERIMORTEM_BENCHMARK(SystemUuid, serialize_x1024) {
  // Format an existing UUID without including generation in the measurement.
  Count value = 0;
  for (Count i = 0; i < uuid_batch; i++) {
    const auto* source = ((U64(i) + uuid_source_offset) & 1) == 0
                             ? &serialize_source_a
                             : &serialize_source_b;
    Benchmark::prevent_optimization(source);
    auto serialized = source->serialize();
    value += serialized[i % serialized.get_size()];
  }

  Benchmark::prevent_optimization(value);
}

PERIMORTEM_BENCHMARK(SystemUuid, deserialize_dashed_x1024) {
  Count value = 0;
  for (Count i = 0; i < uuid_batch; i++) {
    const auto* source = ((U64(i) + uuid_source_offset) & 1) == 0
                             ? &dashed_source_a
                             : &dashed_source_b;
    Benchmark::prevent_optimization(source);
    Uuid uuid;
    uuid.deserialize(*source);
    value ^= uuid.get_value()[0];
  }

  Benchmark::prevent_optimization(value);
}

PERIMORTEM_BENCHMARK(SystemUuid, deserialize_packed_x1024) {
  Count value = 0;
  for (Count i = 0; i < uuid_batch; i++) {
    const auto* source = ((U64(i) + uuid_source_offset) & 1) == 0
                             ? &packed_source_a
                             : &packed_source_b;
    Benchmark::prevent_optimization(source);
    Uuid uuid;
    uuid.deserialize(*source);
    value ^= uuid.get_value()[0];
  }

  Benchmark::prevent_optimization(value);
}
