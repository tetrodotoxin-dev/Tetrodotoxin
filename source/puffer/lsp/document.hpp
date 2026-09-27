// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/record.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"

namespace Puffer::Lsp {

// Document retains protocol text and its discovered Package membership. A
// standalone source owns one lazy analysis, while Package analysis lives on
// the shared Documents session selected by package_root.
class Document {
 public:
  Bool active = False;
  Perimortem::Memory::Dynamic::Bytes uri;
  Perimortem::Memory::Dynamic::Bytes text;
  Perimortem::Memory::Dynamic::Bytes package_root;
  Perimortem::Memory::Dynamic::Bytes logical_route;
  Perimortem::Core::Option<
      Perimortem::Memory::Dynamic::Record<Tetrodotoxin::Source::Lexical::Errors>>
      standalone_errors;
  Perimortem::Core::Option<
      Perimortem::Memory::Dynamic::Record<Tetrodotoxin::Environment::Workspace>>
      standalone_workspace;
};

}  // namespace Puffer::Lsp
