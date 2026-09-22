// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/methods.hpp"

#include <limits.h>
#include <unistd.h>

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/path.hpp"
#include "perimortem/serialization/json/blueprint.hpp"
#include "perimortem/serialization/json/node.hpp"

#include "puffer/lsp/completion.hpp"
#include "puffer/lsp/documents.hpp"
#include "puffer/lsp/hover.hpp"
#include "puffer/lsp/inlay_hints.hpp"
#include "puffer/lsp/rpc/executor.hpp"
#include "puffer/lsp/semantic_tokens.hpp"
#include "tetrodotoxin/formatting/terminal.hpp"
#include "tetrodotoxin/source/lexical/formatter.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Puffer;

auto Lsp::run(View::Bytes pipe) -> S32 {
  Allocator::Arena arena;
  char executable[PATH_MAX];
  const auto length =
      readlink("/proc/self/exe", executable, sizeof(executable));
  if (length <= 0 || length == sizeof(executable)) {
    return 1;
  }
  Perimortem::System::Path binary(
      {reinterpret_cast<const U8*>(executable), Count(length)});
  Managed::Bytes root(arena, binary.get_directory());
  root.concat("/../standard"_view);
  auto repository = Tetrodotoxin::Package::Repository::Repository::create(
      arena, root.get_view());
  if (!repository) {
    return 1;
  }
  Executor executor(*repository);
  executor.execute(pipe);
  return 0;
}

static auto publish_diagnostics(
    Lsp::Documents& documents,
    const Lsp::Rpc::Message& message,
    View::Bytes uri) -> Lsp::Rpc::Response {
  Allocator::Arena& arena = message.get_arena();
  Managed::Vector<Json::Node> diagnostics(arena);
  View::Bytes source = documents.get_text(uri);
  const Lsp::PositionEncoding& encoding = documents.get_position_encoding();
  auto selected = documents.get_diagnostics(uri);
  if (selected) {
    const Tetrodotoxin::Source::Lexical::Errors& errors = selected->get_errors();
    View::Bytes source_name = selected->get_source_name();
    for (Count index = 0; index < errors.get_size(); index++) {
      if (errors.get_source_name(index) != source_name) {
        continue;
      }

      Tetrodotoxin::Source::Lexical::Anchor anchor = errors.get_anchor(index);
      Tetrodotoxin::Source::Lexical::Token token = anchor.get_token();
      Tetrodotoxin::Source::Lexical::Span span = anchor.get_span();
      Count start_offset =
          token ? token.get_offset() : (span ? span.get_offset() : Count(0));
      Count size =
          token ? token.get_size() : (span ? span.get_size() : Count(0));
      auto start = encoding.locate(source, start_offset);
      auto end = encoding.locate(source, start_offset + size);
      if (!start || !end) {
        continue;
      }
      diagnostics.insert(
          Json::Blueprint{
            {
              {"range"_view,
               {
                 {"start"_view,
                  {
                    {"line"_view, start->get_line()},
                    {"character"_view, start->get_character()},
                  }},
                 {"end"_view,
                  {
                    {"line"_view, end->get_line()},
                    {"character"_view, end->get_character()},
                  }},
               }},
              {"severity"_view, S64(1)},
              {"source"_view, "ttx"_view},
              {"message"_view, errors.get_message(index)},
            }}.construct(arena));
    }
  }

  Json::Node published(diagnostics.get_view());
  return Json::Blueprint{
    {
      {"jsonrpc"_view, "2.0"_view},
      {"method"_view, "textDocument/publishDiagnostics"_view},
      {"params"_view,
       {
         {"uri"_view, uri},
         {"diagnostics"_view, published},
       }},
    }}.construct(arena);
}

static auto select_position_encoding(const Lsp::Rpc::Message& message)
    -> Lsp::PositionEncoding {
  // UTF 8 uses the same byte units as TTX source locations. Older clients may
  // omit this list or offer only UTF 16, so the protocol fallback remains the
  // safe default for those sessions.
  Json::Array offered = message
                            .get_params()["capabilities"_view]["general"_view]
                                         ["positionEncodings"_view]
                            .get_array();
  for (const Json::Node& candidate : offered) {
    if (candidate.decode_string(message.get_arena()) == "utf-8"_view) {
      return Lsp::PositionEncoding(Lsp::PositionEncoding::Kind::Utf8);
    }
  }
  return Lsp::PositionEncoding();
}

auto Puffer::Lsp::initialize(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  auto& arena = message.get_arena();
  PositionEncoding encoding = select_position_encoding(message);
  documents.set_position_encoding(encoding);
  return message.report_result(
      Json::Blueprint{
        {
          {"serverInfo"_view,
           {
             {"name"_view, "Tetrodotoxin Language Server"_view},
             {"version"_view, "1.0"_view},
           }},
          {"capabilities"_view,
           {
             {"positionEncoding"_view, encoding.get_name()},
             {"textDocumentSync"_view,
              {
                {"openClose"_view, True},
                {"change"_view, S64(1)},
              }},
             {"hoverProvider"_view, True},
             {"inlayHintProvider"_view, True},
             {"definitionProvider"_view, True},
             {"completionProvider"_view,
              {
                {"triggerCharacters"_view,
                 {
                   "."_view,
                   ":"_view,
                   ">"_view,
                 }},
              }},
             {"documentFormattingProvider"_view, True},
             {"semanticTokensProvider"_view,
              {
                {"legend"_view, Lsp::semantic_legend(arena)},
                {"full"_view, True},
              }},
           }},
        }}.construct(arena));
}

auto Puffer::Lsp::document_formatting(
    Documents& documents,
    const Rpc::Message& message) -> Rpc::Response {
  Allocator::Arena& arena = message.get_arena();
  View::Bytes uri =
      message.get_params()["textDocument"_view]["uri"_view].decode_string(
          arena);
  View::Bytes source = documents.get_text(uri);
  Tetrodotoxin::Source::Lexical::Tokenizer tokenizer(arena, source, uri);
  auto completed = documents.get_completed_monograph(uri);
  Dynamic::Bytes formatted =
      completed
          ? Tetrodotoxin::Formatting::Terminal::format(*completed, tokenizer)
          : Tetrodotoxin::Source::Lexical::Formatter(tokenizer).format();
  View::Bytes formatted_text = arena.proxy(formatted.get_view());
  auto end =
      documents.get_position_encoding().locate(source, source.get_size());
  if (!end) {
    return message.report_result(Json::Node());
  }

  Managed::Vector<Json::Node> edits(arena);
  edits.insert(
      Json::Blueprint{
        {
          {"range"_view,
           {
             {"start"_view,
              {
                {"line"_view, Count(0)},
                {"character"_view, Count(0)},
              }},
             {"end"_view,
              {
                {"line"_view, end->get_line()},
                {"character"_view, end->get_character()},
              }},
           }},
          {"newText"_view, formatted_text},
        }}.construct(arena));
  return message.report_result(Json::Node(edits.get_view()));
}

auto Puffer::Lsp::did_open(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].decode_string(
          message.get_arena());
  const Json::Node text =
      message.get_params()["textDocument"_view]["text"_view];
  View::Bytes decoded = text.decode_string(message.get_arena());
  documents.upsert(uri, decoded);

  Diagnostics::Log::Message<512> log_message(Diagnostics::Log::Level::Info);
  log_message << "File opened: "_view << uri;
  return publish_diagnostics(documents, message, uri);
}

auto Puffer::Lsp::did_change(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].decode_string(
          message.get_arena());
  Json::Array changes = message.get_params()["contentChanges"_view].get_array();
  if (!changes.is_empty()) {
    const Json::Node text = changes[changes.get_size() - 1]["text"_view];
    View::Bytes decoded = text.decode_string(message.get_arena());
    documents.upsert(uri, decoded);
  }

  return publish_diagnostics(documents, message, uri);
}

auto Puffer::Lsp::did_close(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].decode_string(
          message.get_arena());
  documents.erase(uri);

  Diagnostics::Log::Message<512> log_message(Diagnostics::Log::Level::Info);
  log_message << "File closed: "_view << uri;
  return publish_diagnostics(documents, message, uri);
}

auto Puffer::Lsp::did_change_watched_files(
    Documents& documents,
    const Rpc::Message& message) -> Rpc::Response {
  Json::Array changes = message.get_params()["changes"_view].get_array();
  for (const Json::Node& change : changes) {
    View::Bytes uri = change["uri"_view].decode_string(message.get_arena());
    documents.invalidate(uri);
  }

  return {};
}

auto Puffer::Lsp::semantic_tokens(
    Documents& documents,
    const Rpc::Message& message) -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].decode_string(
          message.get_arena());
  View::Bytes source = documents.get_text(uri);
  View::Vector<Tetrodotoxin::Source::Lexical::Token> tokens = documents.get_tokens(uri);
  auto associations = documents.get_associations(uri);
  return message.report_result(
      Lsp::semantic_tokens_for(
          message.get_arena(), source, documents.get_position_encoding(),
          tokens, associations ? &*associations : nullptr));
}

auto Puffer::Lsp::inlay_hints(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  Allocator::Arena& arena = message.get_arena();
  const Json::Node params = message.get_params();
  View::Bytes uri =
      params["textDocument"_view]["uri"_view].decode_string(arena);
  const Json::Node range = params["range"_view];
  const Json::Node start_line = range["start"_view]["line"_view];
  const Json::Node start_character = range["start"_view]["character"_view];
  const Json::Node end_line = range["end"_view]["line"_view];
  const Json::Node end_character = range["end"_view]["character"_view];
  if (uri.is_empty() || !start_line.is_number() ||
      !start_character.is_number() || !end_line.is_number() ||
      !end_character.is_number() || start_line.get_number() < 0 ||
      start_character.get_number() < 0 || end_line.get_number() < 0 ||
      end_character.get_number() < 0) {
    return message.report_result(Json::Node());
  }

  auto associations = documents.get_associations(uri);
  if (!associations) {
    Managed::Vector<Json::Node> empty(arena);
    return message.report_result(Json::Node(empty.get_view()));
  }
  View::Bytes source = documents.get_text(uri);
  const PositionEncoding& encoding = documents.get_position_encoding();
  return message.report_result(
      Lsp::inlay_hints_for(
          arena, source, encoding, *associations,
          PositionEncoding::Position(
              Count(start_line.get_number()),
              Count(start_character.get_number())),
          PositionEncoding::Position(
              Count(end_line.get_number()),
              Count(end_character.get_number()))));
}

auto Puffer::Lsp::hover(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  const Json::Node params = message.get_params();
  const auto uri = params["textDocument"_view]["uri"_view].decode_string(
      message.get_arena());
  const Json::Node line = params["position"_view]["line"_view];
  const Json::Node character = params["position"_view]["character"_view];
  if (uri.is_empty() || !line.is_number() || !character.is_number() ||
      line.get_number() < 0 || character.get_number() < 0) {
    return message.report_result(Json::Node());
  }

  auto semantic = documents.find_semantic(
      uri, PositionEncoding::Position(
               Count(line.get_number()), Count(character.get_number())));
  if (!semantic) {
    return message.report_result(Json::Node());
  }

  return message.report_result(semantic_hover(message.get_arena(), *semantic));
}

auto Puffer::Lsp::definition(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  Allocator::Arena& arena = message.get_arena();
  const Json::Node params = message.get_params();
  View::Bytes uri =
      params["textDocument"_view]["uri"_view].decode_string(arena);
  const Json::Node line = params["position"_view]["line"_view];
  const Json::Node character = params["position"_view]["character"_view];
  if (uri.is_empty() || !line.is_number() || !character.is_number() ||
      line.get_number() < 0 || character.get_number() < 0) {
    return message.report_result(Json::Node());
  }

  PositionEncoding::Position position(
      Count(line.get_number()), Count(character.get_number()));
  auto semantic = documents.find_semantic(uri, position);
  if (!semantic) {
    return message.report_result(Json::Node());
  }

  Option<Tetrodotoxin::Environment::Workspace::AuthoredLocation> location =
      documents.find_acquired_definition(uri, position, *semantic);
  if (!location) {
    location = documents.find_definition(uri, *semantic);
  }
  if (!location) {
    return message.report_result(Json::Node());
  }

  Tetrodotoxin::Source::Lexical::Anchor anchor = location->get_anchor();
  Tetrodotoxin::Source::Lexical::Token focus = anchor.get_token();
  Tetrodotoxin::Source::Lexical::Span span = anchor.get_span();
  Count start_offset =
      focus ? focus.get_offset() : (span ? span.get_offset() : Count(0));
  Count size = focus ? focus.get_size() : (span ? span.get_size() : Count(0));
  const PositionEncoding& encoding = documents.get_position_encoding();
  auto start = encoding.locate(location->get_source_text(), start_offset);
  auto end = encoding.locate(location->get_source_text(), start_offset + size);
  if (!start || !end) {
    return message.report_result(Json::Node());
  }

  Dynamic::Bytes resolved_uri = documents.resolve_uri(*location);
  if (resolved_uri.is_empty()) {
    return message.report_result(Json::Node());
  }
  View::Bytes target_uri = arena.proxy(resolved_uri.get_view());
  return message.report_result(
      Json::Blueprint{
        {
          {"uri"_view, target_uri},
          {"range"_view,
           {
             {"start"_view,
              {
                {"line"_view, start->get_line()},
                {"character"_view, start->get_character()},
              }},
             {"end"_view,
              {
                {"line"_view, end->get_line()},
                {"character"_view, end->get_character()},
              }},
           }},
        }}.construct(arena));
}
