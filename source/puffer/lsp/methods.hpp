// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/utility/pair.hpp"

#include "puffer/lsp/documents.hpp"
#include "puffer/lsp/rpc/executor.hpp"

namespace Puffer::Lsp {

// Temporary socket entry for the retained language server. Repository setup
// stays with LSP while the source executable bootstraps only Build.
auto run(Perimortem::Core::View::Bytes pipe) -> S32;

// Each handler turns one protocol message into a small Documents query. Keeping
// the table here makes the language server's visible surface easy to inspect
// alongside those handlers.
auto initialize(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto document_formatting(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto did_open(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto did_change(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto did_close(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto did_change_watched_files(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto semantic_tokens(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto inlay_hints(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto hover(Documents& documents, const Rpc::Message& message) -> Rpc::Response;
auto completion(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto definition(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;

using Method =
    Perimortem::Utility::Pair<Perimortem::Core::View::Bytes, Rpc::DispatchFunc>;

inline constexpr Perimortem::Core::Static::Vector<Method, 11> method_table = {{
  Method{"initialize"_view, initialize},
  {"textDocument/formatting"_view, document_formatting},
  {"textDocument/didOpen"_view, did_open},
  {"textDocument/didChange"_view, did_change},
  {"textDocument/didClose"_view, did_close},
  {"workspace/didChangeWatchedFiles"_view, did_change_watched_files},
  {"textDocument/semanticTokens/full"_view, semantic_tokens},
  {"textDocument/inlayHint"_view, inlay_hints},
  {"textDocument/hover"_view, hover},
  {"textDocument/completion"_view, completion},
  {"textDocument/definition"_view, definition},
}};

using Executor = Rpc::Executor<method_table>;

}  // namespace Puffer::Lsp
