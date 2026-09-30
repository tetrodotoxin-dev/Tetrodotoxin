// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/dialects/source/snapshot.hpp"
#include "tetrodotoxin/dialects/source/stream.h"
#include "tetrodotoxin/dialects/source/tokens.hpp"

namespace Tetrodotoxin::Dialects::Source {

// Stream's native facade preserves the provider's policy and lifetime. Reading
// a buffer never recovers a Tokenizer or assumes which allocator produced it.
class Stream : public Ttx::Concept::Abstract {
 public:
  using Api = tetrodotoxin_source_stream;
  using Operations = tetrodotoxin_source_stream_ops;
  static constexpr auto contract_id = Perimortem::System::Uuid(
      TETRODOTOXIN_SOURCE_STREAM_ID_HIGH,
      TETRODOTOXIN_SOURCE_STREAM_ID_LOW);
  static constexpr auto lexical_contract = Perimortem::System::Uuid(
      TETRODOTOXIN_SOURCE_LEXICON_ID_HIGH,
      TETRODOTOXIN_SOURCE_LEXICON_ID_LOW);
  static auto accept(Api value) -> Bool {
    return value.operations && value.operations->input &&
           value.operations->tokens && value.operations->lexicon &&
           Abstract::accept({value.source, &value.operations->abstract});
  }
  explicit constexpr Stream(Api value)
      : Abstract(value.source, value.operations->abstract) {}
  constexpr auto get_abi() const -> Api {
    auto value = Abstract::get_abi();
    return {
      value.source, reinterpret_cast<const Operations*>(value.operations)};
  }
  auto get_input() const -> tetrodotoxin_source_snapshot {
    auto value = get_abi();
    return value.operations->input(value.source);
  }
  auto get_tokens() const -> Tokens {
    auto value = get_abi();
    return Tokens(value.operations->tokens(value.source));
  }
  auto get_lexicon() const -> Perimortem::System::Uuid {
    auto value = get_abi();
    return Perimortem::System::Uuid(value.operations->lexicon(value.source));
  }
  auto get_source_text() const -> Perimortem::Core::View::Bytes {
    auto value = get_input().text;
    return {value.data, value.size};
  }
  auto get_source_path() const -> Perimortem::Core::View::Bytes {
    auto value = get_input().path;
    return {value.data, value.size};
  }
  template <typename Owner>
  static auto provide(const Owner& owner) -> Stream {
    static const Operations operations{
      *Abstract::provide(owner).get_abi().operations,
      [](const void* source) {
        return static_cast<const Owner*>(source)->get_input();
      },
      [](const void* source) {
        return static_cast<const Owner*>(source)->get_token_buffer();
      },
      [](const void* source) {
        return static_cast<perimortem_uuid>(
            static_cast<const Owner*>(source)->get_lexicon());
      }};
    return Stream({&owner, &operations});
  }
};
}  // namespace Tetrodotoxin::Dialects::Source

TTX_DATA_RECORD(
    tetrodotoxin_source_stream_ops,
    TTX_DATA_MEMBER(tetrodotoxin_source_stream_ops, abstract),
    TTX_DATA_MEMBER(tetrodotoxin_source_stream_ops, input),
    TTX_DATA_MEMBER(tetrodotoxin_source_stream_ops, tokens),
    TTX_DATA_MEMBER(tetrodotoxin_source_stream_ops, lexicon));
TTX_DATA_RECORD(
    tetrodotoxin_source_stream,
    TTX_DATA_MEMBER(tetrodotoxin_source_stream, source),
    TTX_DATA_MEMBER(tetrodotoxin_source_stream, operations));
