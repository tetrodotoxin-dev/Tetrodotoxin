// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/serialization/json/node.hpp"

#include "puffer/lsp/position_encoding.hpp"
#include "tetrodotoxin/source/lexical/associations.hpp"

namespace Puffer::Lsp {

// A Call already remembers how each argument fitted its parameter. Reading that
// evidence gives the editor useful labels without rebuilding signature
// matching or keeping a second map beside the semantic graph.
auto inlay_hints_for(
    Perimortem::Memory::Allocator::Arena& arena,
    Perimortem::Core::View::Bytes source,
    const PositionEncoding& encoding,
    const Tetrodotoxin::Source::Lexical::Associations& associations,
    const PositionEncoding::Position& start,
    const PositionEncoding::Position& end)
    -> Perimortem::Serialization::Json::Node;

}  // namespace Puffer::Lsp
