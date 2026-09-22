// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/benchmark.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/serialization/json/blueprint.hpp"
#include "perimortem/serialization/json/node.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Serialization;
using namespace Validation;

static Static::Bytes<1 << 15> json_data;
static Writer::Textual json_text(json_data);
static constexpr Count json_batch = 1024;

static auto load_json(View::Bytes source_path) -> void {
  auto source = File::read(source_path);
  if (!source) {
    Diagnostics::Log::fatal("Unable to load JSON benchmark source."_view);
  }

  json_text.set_pointer(0);
  json_text << *source;
}

static Harness JsonBlueprint = {
  .name = "Json"_view,
};

PERIMORTEM_BENCHMARK(JsonBlueprint, blueprint_x1024) {
  Count size = 0;
  Allocator::Arena arena;
  for (Count i = 0; i < json_batch; i++) {
    Json::Node node = Json::Blueprint{
      {
        {"jsonrpc"_view, "2.0"_view},
        {"id"_view, 1},
        {"result"_view,
         {
           {"name"_view, "ttx"_view},
           {"version"_view, "1.0"_view},
         }},
      }}.construct(arena);
    size += node.get_size();
    arena.reset();
  }

  Benchmark::prevent_optimization(size);
}

static Harness JsonSmall = {
  .name = "Json"_view,
  .init = []() { load_json("validation/data/json/test.json"_view); },
};

PERIMORTEM_BENCHMARK(JsonSmall, small_x1024) {
  Count size = 0;

  Allocator::Arena arena;
  for (Count i = 0; i < json_batch; i++) {
    Json::Node node;
    node.parse(arena, json_text);
    size += node.get_size();
    arena.reset();
  }

  Benchmark::prevent_optimization(size);
}

static Harness JsonTokenizeRpc = {
  .name = "Json"_view,
  .init = []() { load_json("validation/data/json/tokenize_rpc.json"_view); },
};

PERIMORTEM_BENCHMARK(JsonTokenizeRpc, tokenize_rpc_x1024) {
  Count size = 0;

  Allocator::Arena arena;
  for (Count i = 0; i < json_batch; i++) {
    Json::Node node;
    node.parse(arena, json_text);
    size += node.get_size();
    arena.reset();
  }

  Benchmark::prevent_optimization(size);
}

static Harness JsonInitRpc = {
  .name = "Json"_view,
  .init = []() { load_json("validation/data/json/init_rpc.json"_view); },
};

PERIMORTEM_BENCHMARK(JsonInitRpc, jsonrpc_init_x1024) {
  Count size = 0;

  Allocator::Arena arena;
  for (Count i = 0; i < json_batch; i++) {
    Json::Node node;
    node.parse(arena, json_text);
    size += node.get_size();
    arena.reset();
  }

  Benchmark::prevent_optimization(size);
}
