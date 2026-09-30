// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/dialects/source/stream.hpp"

namespace Tetrodotoxin::Dialects::Source {

// Tokenizer partitions borrowed source bytes into the concrete TTX vocabulary.
// Its Arena owns the compact token buffer. Unknown spans retain their authored
// bytes for editors. A coordinate or token length outside the compact profile
// rejects the whole buffer before narrowing any field.
class Tokenizer {
 public:
  Tokenizer(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source_text,
      Perimortem::Core::View::Bytes source_path)
      : arena(arena), source_text(source_text), source_path(source_path) {
    parse();
  }
  auto is_valid() const -> Bool { return !tokens.is_empty(); }
  auto is_empty() const -> Bool { return tokens.get_size() <= 1; }
  auto get_tokens() const -> Perimortem::Core::View::Vector<Token> {
    return tokens;
  }
  auto get_source_text() const -> Perimortem::Core::View::Bytes {
    return source_text;
  }
  auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }
  auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }
  auto get_input() const -> tetrodotoxin_source_snapshot {
    return {
      {source_path.get_data(), source_path.get_size()},
      {source_text.get_data(), source_text.get_size()}};
  }
  auto get_token_buffer() const -> tetrodotoxin_source_tokens {
    return {
      &Ttx::Data::Form::Compiled<Ttx::Data::Form::Native<
          tetrodotoxin_source_token>::reference>::get_representation(),
      reinterpret_cast<const U8*>(tokens.get_data()),
      tokens.get_size() * sizeof(Token), tokens.get_size()};
  }
  auto get_lexicon() const -> Perimortem::System::Uuid {
    return Stream::lexical_contract;
  }
  auto get_data() const -> Perimortem::Core::View::Bytes { return source_text; }
  auto get_stream() const -> Stream { return Stream::provide(*this); }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage target) const
      -> Ttx::Semantic::Negotiation::Binding::Status;

 private:
  auto parse() -> void;
  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Core::View::Bytes source_text;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Vector<Token> tokens;
};
}  // namespace Tetrodotoxin::Dialects::Source
