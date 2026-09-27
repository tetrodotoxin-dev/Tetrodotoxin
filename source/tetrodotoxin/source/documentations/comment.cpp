// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/documentations/comment.hpp"

using namespace Tetrodotoxin::Source;

auto Documentations::Comment::get_empty() -> const Comment& {
  static constexpr Comment comment{Perimortem::Core::View::Bytes()};
  return comment;
}
