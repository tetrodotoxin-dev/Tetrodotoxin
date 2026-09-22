// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/completion.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/json/blueprint.hpp"

#include "puffer/lsp/hover.hpp"
#include "tetrodotoxin/source/none.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/addressable.hpp"
#include "tetrodotoxin/source/callable.hpp"
#include "tetrodotoxin/source/type.hpp"

using namespace Perimortem;
using namespace Perimortem::Serialization;
using namespace Puffer;
using namespace Ttx;

static auto completion_item(
    Memory::Allocator::Arena& arena,
    const Tetrodotoxin::Source::Abstract& candidate) -> Json::Node {
  S64 kind = 6;
  if (candidate.is<Tetrodotoxin::Source::Callable>()) {
    kind = 3;
  } else if (candidate.is<Tetrodotoxin::Source::Addressable>()) {
    kind = 5;
  } else if (candidate.is<Tetrodotoxin::Source::Type>()) {
    kind = 7;
  }

  Json::Node hover = Lsp::semantic_hover(arena, candidate);
  return Json::Blueprint{
    {
      {"label"_view, candidate.get_name()},
      {"kind"_view, kind},
      {"documentation"_view, hover["contents"_view]},
    }}.construct(arena);
}

static constexpr auto is_suffix(Core::View::Bytes source) -> Bool {
  for (Count index = 0; index < source.get_size(); index++) {
    U8 byte = source[index];
    Bool name = (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
                (byte >= '0' && byte <= '9') || byte == '_';
    if (!name && byte != ' ' && byte != '\t') {
      return False;
    }
  }
  return True;
}

static auto select_authority(
    const Tetrodotoxin::Source::Abstract& authored,
    Tetrodotoxin::Source::Lexical::Code::Type operation) -> const Tetrodotoxin::Source::Abstract& {
  auto addressable = authored.select<Tetrodotoxin::Source::Addressable>();
  if (addressable) {
    return addressable->get_type().resolve_concept("instance"_view);
  }

  auto type = authored.select<Tetrodotoxin::Source::Type>();
  if (type) {
    return type->resolve_concept("static"_view);
  }

  const Tetrodotoxin::Source::Abstract& authored_static =
      authored.resolve_concept("static"_view);
  if (!authored_static.is<Tetrodotoxin::Source::Unknown>() &&
      !authored_static.is<Tetrodotoxin::Source::None>()) {
    return authored_static;
  }

  const Tetrodotoxin::Source::Abstract& represented = authored.resolve();
  addressable = represented.select<Tetrodotoxin::Source::Addressable>();
  if (addressable) {
    return addressable->get_type().resolve_concept("instance"_view);
  }
  type = represented.select<Tetrodotoxin::Source::Type>();
  if (type) {
    return type->resolve_concept("static"_view);
  }

  const Tetrodotoxin::Source::Abstract& selected_type = authored.get_type().resolve();
  return operation == Tetrodotoxin::Source::Lexical::Code::Type::TypeAccessOp
             ? selected_type.resolve_concept("static"_view)
             : selected_type.resolve_concept("instance"_view);
}

static auto accepts(
    const Tetrodotoxin::Source::Abstract& candidate,
    Tetrodotoxin::Source::Lexical::Code::Type operation) -> Bool {
  if (operation == Tetrodotoxin::Source::Lexical::Code::Type::CallOp) {
    return candidate.is<Tetrodotoxin::Source::Callable>();
  }
  if (operation == Tetrodotoxin::Source::Lexical::Code::Type::AddressOp) {
    return candidate.is<Tetrodotoxin::Source::Addressable>();
  }
  return !candidate.is<Tetrodotoxin::Source::Callable>() &&
         !candidate.is<Tetrodotoxin::Source::Addressable>();
}

static auto complete(
    Memory::Allocator::Arena& arena,
    Memory::Managed::Vector<Json::Node>& items,
    const Tetrodotoxin::Source::Abstract& authority,
    Tetrodotoxin::Source::Lexical::Code::Type operation) -> void {
  if (authority.is<Tetrodotoxin::Source::Unknown>() || authority.is<Tetrodotoxin::Source::None>()) {
    return;
  }

  auto receive = [&](Core::View::Bytes, const Tetrodotoxin::Source::Abstract& candidate) {
    if (accepts(candidate, operation)) {
      items.insert(completion_item(arena, candidate));
    }
  };
  authority.visit_concepts(Tetrodotoxin::Source::Abstract::Visitor(receive));
}

auto Puffer::Lsp::completion(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  Memory::Allocator::Arena& arena = message.get_arena();
  const Json::Node params = message.get_params();
  Core::View::Bytes uri =
      params["textDocument"_view]["uri"_view].decode_string(arena);
  const Json::Node line = params["position"_view]["line"_view];
  const Json::Node character = params["position"_view]["character"_view];
  Memory::Managed::Vector<Json::Node> items(arena);
  if (uri.is_empty() || !line.is_number() || !character.is_number() ||
      line.get_number() < 0 || character.get_number() < 0) {
    return message.report_result(Json::Node(items.get_view()));
  }

  Core::View::Bytes source = documents.get_text(uri);
  auto offset = documents.get_position_encoding().find_offset(
      source, PositionEncoding::Position(
                  Count(line.get_number()), Count(character.get_number())));
  auto associations = documents.get_associations(uri);
  auto tokens = documents.get_tokens(uri);
  if (!offset || !associations) {
    return message.report_result(Json::Node(items.get_view()));
  }

  Count operation_index = Count(-1);
  for (Count index = 0; index < tokens.get_size(); index++) {
    Tetrodotoxin::Source::Lexical::Token token = tokens.get_data()[index];
    if (!token || Count(token.get_offset()) + token.get_size() > *offset) {
      continue;
    }
    auto code = token.get_code().get_type();
    if (code == Tetrodotoxin::Source::Lexical::Code::Type::AddressOp ||
        code == Tetrodotoxin::Source::Lexical::Code::Type::TypeAccessOp ||
        code == Tetrodotoxin::Source::Lexical::Code::Type::CallOp) {
      operation_index = index;
    }
  }
  if (operation_index == Count(-1) || operation_index == 0) {
    return message.report_result(Json::Node(items.get_view()));
  }

  Tetrodotoxin::Source::Lexical::Token operation = tokens.get_data()[operation_index];
  Count operation_end = Count(operation.get_offset()) + operation.get_size();
  if (*offset < operation_end ||
      !is_suffix(source.slice(operation_end, *offset - operation_end))) {
    return message.report_result(Json::Node(items.get_view()));
  }

  Tetrodotoxin::Source::Lexical::Token receiver = tokens.get_data()[operation_index - 1];
  auto semantic = associations->find_at(
      Count(receiver.get_offset()) + receiver.get_size() - 1);
  if (semantic) {
    const Tetrodotoxin::Source::Abstract& authority =
        select_authority(*semantic, operation.get_code().get_type());
    complete(arena, items, authority, operation.get_code().get_type());
  }
  return message.report_result(Json::Node(items.get_view()));
}
