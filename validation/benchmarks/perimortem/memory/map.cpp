// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/memory/dynamic/map.hpp"

#include "validation/benchmark.hpp"

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/system/random.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Validation;

constexpr Count max_key_count = 1 << 16;
static Static::Vector<S32, max_key_count> lookup_keys;
static Static::Vector<S32, max_key_count> missing_lookup_keys;

static auto populate_lookup_keys() -> void {
  static Bool populated = False;
  if (populated) {
    return;
  }

  for (Count i = 0; i < max_key_count; i++) {
    lookup_keys[i] = S32(i);
    missing_lookup_keys[i] = S32(i);
  }

  populated = True;
}

static Harness MapS32s = {
  .name = "Map Performance"_view,
  .init = populate_lookup_keys,
};

template <Count values, Bool lookup>
static auto map_test() -> void {
  Dynamic::Map<S32, S32> local_map(values);
  for (Count i = 0; i < values; i++) {
    local_map.insert(lookup_keys[i], S32(i));
  }

  S32 accumulator = local_map.get_size();
  if constexpr (lookup) {
    Benchmark::start_time();
    accumulator = 0;
    for (Count i = 0; i < values; i++) {
      accumulator += local_map.at(lookup_keys[i % values]);
    }
  }

  Benchmark::end_time();
  Benchmark::prevent_optimization(accumulator);
}

#define MAP_INT_TEST(count, var)                        \
  PERIMORTEM_BENCHMARK(MapS32s, var##_##count##_ints) { \
    map_test<count, var>();                             \
  }

constexpr auto lookup = True;
constexpr auto insert = False;

MAP_INT_TEST(256, lookup);
MAP_INT_TEST(1024, lookup);

MAP_INT_TEST(256, insert);
MAP_INT_TEST(1024, insert);

static Harness MapWorkloads = {
  .name = "Map Workloads"_view,
  .init = populate_lookup_keys,
};

static auto pointer_key(Count index, Bool missing = False) -> const S32* {
  if (missing) {
    return missing_lookup_keys.get_data() + index;
  }

  return lookup_keys.get_data() + index;
}

template <Count values, Bool misses>
static auto pointer_lookup_test() -> void {
  Dynamic::Map<const S32*, Count> map(values);
  for (Count i = 0; i < values; i++) {
    map.insert(pointer_key(i), i);
  }

  Count accumulator = 0;
  Benchmark::start_time();
  for (Count i = 0; i < values; i++) {
    const S32* key = pointer_key(i, misses);
    auto entry = map.find(key);
    accumulator += entry ? (*entry).value : 1;
  }

  Benchmark::end_time();
  Benchmark::prevent_optimization(accumulator);
}

template <Count values, Bool reserve, Bool duplicate>
static auto pointer_insert_test() -> void {
  Dynamic::Map<const S32*, Count> map;
  if constexpr (reserve) {
    map.ensure_capacity(values);
  }

  if constexpr (duplicate) {
    for (Count i = 0; i < values; i++) {
      map.insert(pointer_key(i), i);
    }
  }

  Benchmark::start_time();
  for (Count i = 0; i < values; i++) {
    map.insert(pointer_key(i), i + 1);
  }

  Benchmark::end_time();
  Count size = map.get_size();
  Benchmark::prevent_optimization(size);
}

#define MAP_POINTER_LOOKUP(count, kind)                          \
  PERIMORTEM_BENCHMARK(MapWorkloads, pointer_##kind##_##count) { \
    pointer_lookup_test<count, kind == miss>();                  \
  }

#define MAP_POINTER_INSERT(count, reserve, duplicate)                   \
  PERIMORTEM_BENCHMARK(                                                 \
      MapWorkloads, pointer_insert_##count##_##reserve##_##duplicate) { \
    pointer_insert_test<count, reserve, duplicate>();                   \
  }

constexpr auto hit = False;
constexpr auto miss = True;
constexpr auto growing = False;
constexpr auto reserved = True;
constexpr auto unique = False;
constexpr auto duplicate = True;

MAP_POINTER_LOOKUP(32, hit);
MAP_POINTER_LOOKUP(32, miss);
MAP_POINTER_LOOKUP(900, hit);
MAP_POINTER_LOOKUP(900, miss);
MAP_POINTER_LOOKUP(16384, hit);
MAP_POINTER_LOOKUP(16384, miss);

MAP_POINTER_INSERT(32, growing, unique);
MAP_POINTER_INSERT(32, reserved, unique);
MAP_POINTER_INSERT(900, growing, unique);
MAP_POINTER_INSERT(900, reserved, unique);
MAP_POINTER_INSERT(16384, growing, unique);
MAP_POINTER_INSERT(16384, reserved, unique);
MAP_POINTER_INSERT(32, reserved, duplicate);
MAP_POINTER_INSERT(900, reserved, duplicate);
MAP_POINTER_INSERT(921, reserved, duplicate);

static constexpr Pair<View::Bytes, Count> keyword_source[] = {
  {"as"_view, 0},         {"if"_view, 1},          {"for"_view, 2},
  {"new"_view, 3},        {"else"_view, 4},        {"func"_view, 5},
  {"init"_view, 6},       {"self"_view, 7},        {"true"_view, 8},
  {"alias"_view, 9},      {"debug"_view, 10},      {"error"_view, 11},
  {"false"_view, 12},     {"using"_view, 13},      {"while"_view, 14},
  {"entity"_view, 15},    {"object"_view, 16},     {"return"_view, 17},
  {"struct"_view, 18},    {"library"_view, 19},    {"on_load"_view, 20},
  {"package"_view, 21},   {"warning"_view, 22},    {"testing"_view, 23},
  {"insert_x"_view, 24},  {"deletion"_view, 25},   {"removals"_view, 26},
  {"log_print"_view, 27}, {"log_print2"_view, 28}, {"x"_view, 29},
  {"y"_view, 30},         {"main"_view, 31},
};

static constexpr Pair<View::Bytes, Count> non_keyword_source[] = {
  {"af"_view, 0},         {"1f"_view, 1},          {"f2r"_view, 2},
  {"naw"_view, 3},        {"el.e"_view, 4},        {"fu1c"_view, 5},
  {"ini?"_view, 6},       {"sel2"_view, 7},        {"t4ue"_view, 8},
  {"aliis"_view, 9},      {"d1bug"_view, 10},      {"err7r"_view, 11},
  {"aalse"_view, 12},     {"us0ng"_view, 13},      {"wzile"_view, 14},
  {"entnty"_view, 15},    {"ofjfct"_view, 16},     {"ret9rn"_view, 17},
  {"strzct"_view, 18},    {"lixrary"_view, 19},    {"on_doad"_view, 20},
  {"pa!kag!"_view, 21},   {"war_ing"_view, 22},    {"testi1g"_view, 23},
  {"in3ert_x"_view, 24},  {"de-_tion"_view, 25},   {"remova3s"_view, 26},
  {"log  rint"_view, 27}, {"log_print3"_view, 28}, {"z"_view, 29},
  {"a"_view, 30},         {"nain"_view, 31},
};
static constexpr Count keyword_count = Data::array_size(keyword_source);

using KeywordTable = Table<Count, keyword_source>;

static Static::Vector<const View::Bytes*, max_key_count> lookup_scramble;
static auto create_scramble(Count mask) -> void {
  // Create a chaotic lookup order with a controlled percentage of misses.
  for (Count i = 0; i < max_key_count; i++) {
    auto index = Random::generate();
    if (index & mask) {
      // Miss
      lookup_scramble[i] = &non_keyword_source[index % keyword_count].key;
    } else {
      // Hit
      lookup_scramble[i] = &keyword_source[index % keyword_count].key;
    }
  }
}

template <Count values, Count mask>
static auto keyword_test() -> void {
  Dynamic::Map<View::Bytes, S32> local_map(keyword_count);
  for (Count i = 0; i < keyword_count; i++) {
    local_map.insert(keyword_source[i].key, keyword_source[i].value);
  }

  create_scramble(mask);
  S32 accumulator = local_map.get_size();

  Benchmark::start_time();
  for (Count i = 0; i < values; i++) {
    accumulator += local_map.find_or_default(*lookup_scramble[i], -1);
  }

  Benchmark::prevent_optimization(accumulator);
  Benchmark::end_time();
}

template <Count values, Count mask>
static auto keyword_table() -> void {
  create_scramble(mask);
  S32 accumulator = 0;

  Benchmark::start_time();
  for (Count i = 0; i < values; i++) {
    accumulator += KeywordTable::find_or_default(*lookup_scramble[i], -1);
  }

  Benchmark::prevent_optimization(accumulator);
  Benchmark::end_time();
}

static Harness MapKeywords = {
  .name = "Keyword Lookup"_view,
};

#define MAP_KEYWORD_TEST(count, mask)                         \
  PERIMORTEM_BENCHMARK(MapKeywords, count##_##mask##_##map) { \
    keyword_test<count, mask>();                              \
  }

#define MAP_TABLE_TEST(count, mask)                             \
  PERIMORTEM_BENCHMARK(MapKeywords, count##_##mask##_##table) { \
    keyword_table<count, mask>();                               \
  }

#define MAP_KEYWORD_TEST_RANGE(count, mask) \
  MAP_KEYWORD_TEST(count, mask);            \
  MAP_TABLE_TEST(count, mask);

// Compare complete hits with roughly one hit per 1024 source keys because
// table behavior changes with hit rate.
// hit_1000 equals 100.0% source keys.
// hit_0001 equals about 0.1% source keys.
constexpr auto hit_1000 = Count(0b00000000'00000000);
constexpr auto hit_0001 = Count(0b00000011'11111111);

MAP_KEYWORD_TEST_RANGE(4096, hit_1000);
MAP_KEYWORD_TEST_RANGE(4096, hit_0001);
MAP_KEYWORD_TEST_RANGE(16384, hit_1000);
MAP_KEYWORD_TEST_RANGE(16384, hit_0001);
