// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/terminal/spirv/failure.hpp"
#include "tetrodotoxin/terminal/spirv/module/body.hpp"
#include "tetrodotoxin/terminal/spirv/module/constants.hpp"
#include "tetrodotoxin/terminal/spirv/module/ids.hpp"
#include "tetrodotoxin/terminal/spirv/module/interface.hpp"
#include "tetrodotoxin/terminal/spirv/module/types.hpp"
#include "tetrodotoxin/terminal/spirv/products.hpp"
#include "tetrodotoxin/terminal/spirv/request.hpp"

namespace Tetrodotoxin::Terminal::Spirv::Module {

// Program coordinates the ordered SPIR V sections for one request. Its helper
// owners share ids and derived physical facts without acquiring a second
// semantic body, Type graph, or Shader relationship inventory.
class Program {
 public:
  Program(Perimortem::Memory::Allocator::Arena& arena, const Request& request)
      : arena(arena),
        request(request),
        types(ids),
        constants(ids, types),
        interface(arena, ids, types),
        body(ids, types, constants, interface, request) {}

  auto compile() -> Perimortem::Utility::Result<Products, Failure>;

 private:
  auto report_failure(Perimortem::Core::View::Bytes message) const -> void;

  Perimortem::Memory::Allocator::Arena& arena;
  const Request& request;
  Ids ids;
  Types types;
  Constants constants;
  Interface interface;
  Body body;
};

}  // namespace Tetrodotoxin::Terminal::Spirv::Module
