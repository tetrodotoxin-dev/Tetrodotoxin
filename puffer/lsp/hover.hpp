// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/serialization/json/node.hpp"

#include "tetrodotoxin/source/abstract.hpp"

namespace Puffer::Lsp {

// Builds presentation directly from one selected semantic identity. The output
// Arena owns only JSON and Markdown bytes, never another semantic model.
auto semantic_hover(
    Perimortem::Memory::Allocator::Arena& arena,
    const Tetrodotoxin::Source::Abstract& semantic)
    -> Perimortem::Serialization::Json::Node;

}  // namespace Puffer::Lsp
