// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/interpreter/bridge.hpp"

#include "tetrodotoxin/library/interpreter/type_reference.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

static auto read_policy(
    View::Vector<Tetrodotoxin::Language::Attribute> attributes,
    View::Bytes key) -> Option<View::Bytes> {
  for (const Tetrodotoxin::Language::Attribute& attribute : attributes) {
    if (attribute.get_key() == key) {
      const View::Bytes* value = attribute.get_value().find<View::Bytes>();
      return value ? Option<View::Bytes>(*value) : Option<View::Bytes>();
    }
  }
  return {};
}

static auto validate_attributes(
    Cursor& cursor,
    View::Vector<Tetrodotoxin::Language::Attribute> attributes) -> Bool {
  Bool valid = True;
  for (Count i = 0; i < attributes.get_size(); i++) {
    const auto& attribute = attributes.get_data()[i];
    View::Bytes key = attribute.get_key();
    if ((key != "direction"_view && key != "marshal"_view &&
         key != "sync"_view) ||
        !attribute.get_value().find<View::Bytes>()) {
      cursor.create_expression_error(
          attribute.get_anchor(),
          "Shader Bridge Attributes use named direction, marshal, and sync policies."_view);
      valid = False;
    }
    for (Count prior = 0; prior < i; prior++) {
      if (attributes.get_data()[prior].get_key() == key) {
        cursor.create_expression_error(
            attribute.get_anchor(),
            "Shader Bridge cannot repeat one policy Attribute."_view);
        valid = False;
      }
    }
  }
  return valid;
}

auto Interpreter::Bridge::parse(
    Shader::Language::Monograph& monograph,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  auto attributes = definition.get_attributes();
  BAIL_IF(!validate_attributes(cursor, attributes));
  if (definition.get_authored().get_name().get_code() != Code::Type::Addressable) {
    cursor.create_token_error(
        definition.get_authored().get_name(),
        "Shader Bridges use one addressable relationship name."_view);
    return False;
  }
  if (definition.get_visibility() !=
      Tetrodotoxin::Language::Visibility::Public) {
    cursor.create_token_error(
        definition.get_authored().get_visibility(),
        "Shader Bridges use public visibility."_view);
    return False;
  }
  if (!definition.get_authored().get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_authored().get_modifiers().get_data()[0],
        "Shader Bridges do not accept evaluation modifiers."_view);
    return False;
  }
  Token qualifier = cursor.consume();
  auto cpu = Library::Interpreter::TypeReference::parse(monograph, cursor);
  BAIL_IF(!cpu);
  BAIL_IF(!cursor.require(
      Code::Type::CallOp,
      "Shader Bridge requires `->` between CPU and GPU Types."_view));
  auto gpu = Library::Interpreter::TypeReference::parse(monograph, cursor);
  BAIL_IF(!gpu);
  Token closing = cursor.require(
      Code::Type::EndStatement,
      "Shader Bridge requires one trailing `;`."_view);
  BAIL_IF(!closing);

  auto direction_name = read_policy(attributes, "direction"_view);
  auto marshal_name = read_policy(attributes, "marshal"_view);
  auto sync_name = read_policy(attributes, "sync"_view);
  if (!direction_name || !marshal_name || !sync_name) {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Shader Bridge requires `@direction`, `@marshal`, and `@sync` policies."_view);
    return False;
  }

  Shader::Language::Bridge::Direction direction;
  if (*direction_name == "upload"_view) {
    direction = Shader::Language::Bridge::Direction::Upload;
  } else if (*direction_name == "download"_view) {
    direction = Shader::Language::Bridge::Direction::Download;
  } else if (*direction_name == "bidirectional"_view) {
    direction = Shader::Language::Bridge::Direction::Bidirectional;
  } else {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Shader Bridge direction is `upload`, `download`, or `bidirectional`."_view);
    return False;
  }

  Shader::Language::Bridge::Marshaling marshaling;
  if (*marshal_name == "identity"_view) {
    marshaling = Shader::Language::Bridge::Marshaling::Identity;
  } else if (*marshal_name == "copy"_view) {
    marshaling = Shader::Language::Bridge::Marshaling::Copy;
  } else if (*marshal_name == "pack"_view) {
    marshaling = Shader::Language::Bridge::Marshaling::Pack;
  } else {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Shader Bridge marshaling is `identity`, `copy`, or `pack`."_view);
    return False;
  }

  Shader::Language::Bridge::Synchronization synchronization;
  if (*sync_name == "none"_view) {
    synchronization = Shader::Language::Bridge::Synchronization::None;
  } else if (*sync_name == "submission"_view) {
    synchronization = Shader::Language::Bridge::Synchronization::Submission;
  } else if (*sync_name == "frame"_view) {
    synchronization = Shader::Language::Bridge::Synchronization::Frame;
  } else {
    cursor.create_expression_error(
        definition.get_authored().get_anchor(),
        "Shader Bridge synchronization is `none`, `submission`, or `frame`."_view);
    return False;
  }

  auto& authored = definition.get_authored();
  authored.set_anchor(Anchor::create(
      qualifier, Span(authored.get_anchor().get_span().get_start(), closing)));
  auto& bridge = Shader::Language::Bridge::create(
      cursor.get_arena(), definition, *cpu, *gpu, direction, marshaling,
      synchronization);
  cursor.get_associations().create(
      Anchor::create(Span(definition.get_authored().get_name())), bridge);
  return monograph.retain_bridge(bridge);
}
