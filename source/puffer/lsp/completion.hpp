// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "puffer/lsp/documents.hpp"
#include "puffer/lsp/rpc/message.hpp"

namespace Puffer::Lsp {

auto completion(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;

}  // namespace Puffer::Lsp
