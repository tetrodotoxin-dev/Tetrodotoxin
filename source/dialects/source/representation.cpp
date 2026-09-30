// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialects/source/capabilities/parse.hpp"
#include "tetrodotoxin/dialects/source/input.hpp"
#include "tetrodotoxin/dialects/source/stream.hpp"

using namespace Ttx::Data::Form;

extern "C" const ttx_representation*
    tetrodotoxin_source_input_representation() {
  return &Compiled<
      Native<tetrodotoxin_source_input>::reference>::get_representation();
}

extern "C" const ttx_representation*
    tetrodotoxin_source_stream_representation() {
  return &Ttx::Semantic::Negotiation::Binding::representation<
      Tetrodotoxin::Dialects::Source::Stream>();
}

extern "C" const ttx_representation*
    tetrodotoxin_source_token_representation() {
  return &Compiled<
      Native<tetrodotoxin_source_token>::reference>::get_representation();
}
extern "C" const ttx_representation*
    tetrodotoxin_source_diagnostics_representation() {
  return &Ttx::Semantic::Negotiation::Binding::representation<
      Tetrodotoxin::Dialects::Source::Capabilities::Diagnostics>();
}
extern "C" const ttx_representation*
    tetrodotoxin_source_parse_representation() {
  return &Ttx::Semantic::Negotiation::Binding::representation<
      Tetrodotoxin::Dialects::Source::Capabilities::Parse>();
}
