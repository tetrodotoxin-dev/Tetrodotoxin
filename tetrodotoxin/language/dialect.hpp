// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::Language {

// TTX (Toolchain Text Extensions) encode a given instruction stream into a sort
// of command buffer. While normally this is a richer semantic representation of
// a tokenized source file, TTX token streams can come from arbitrary sources.
//
// Dialects provide a way to execute a variable range of a given token stream to
// produce any number of side effects + a single optional ABI specified object.
//
// It's important to note that `execute` is used here to differentiate Dialect
// functionality from that of a typical parser. There are some Dialects that use
// the stream to create something that resembles an AST, but this is not the
// common case.
//
// `interpret` provides the main entry and exit point for plugin Terminals.
// Since Terminals leave the graph that means any two Dialects that produce the
// exact same Monograph structure for every possible input are considered to be
// simulacra and can be substituted just like any other Abstract.
class Dialect : public Tetrodotoxin::Source::Abstract {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Source::Abstract);

  virtual ~Dialect() = default;

  // The interface for driving the actual Dialect after it's been selected. It's
  // the main hook that allows plugin's to drive side effect behavior by using a
  // TTX stream as it's command buffer.
  //
  // The Dialect can return any Monograph (including forwarding Monographs from
  // other Dialects). While other data can be exchanged, all Abstracts reachable
  // from the Monograph returned are the only ABI stable promises Tetrodotoxin
  // provides. Any sidecars are undefined behavior to Tetrodotoxin.
  //
  // Cursor contains the location the Dialect should start reading from. It has
  // access to the `Span(cursor, Code::Terimnal)`. Reading any tokens before the
  // cursor's position is undefined behavior, even if the token itself is well
  // defined. This allows for Dialects to host sub-Dialects which is the base
  // case for how the Tetrodotoxin::Toolchain drives source by interpreting the
  // header of the TTX stream up until it finds the command to use a Dialect.
  //
  // TODO: API is still work in progress but this seems about right minus some
  // context shuffling as we work out plugins.
  virtual auto interpret(
      Tetrodotoxin::Source::Lexical::Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation,
      const Tetrodotoxin::Source::Lexical::Anchor& source_anchor,
      Tetrodotoxin::Source::Abstract& context)
      -> Perimortem::Core::Option<Monograph&> = 0;

  // Uses the Dialect to encode the Abstract in two steps:
  //
  // 1. Compress the Abstract if possible into a domain specific simulacra.
  // 2. Encode that simulacra into an arbitrary byte array.
  //
  // This is essentially serialization but gives the Dialect the promise that it
  // will be the only one ever asked to decode the bytes to regenerate the right
  // simulacra.
  virtual auto encode(const Tetrodotoxin::Source::Abstract& abstract) const
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes> {
    return {};
  }

  // Uses the Dialect to create a Abstract simulacra that represents all routes
  // that the original encoded Abstract could produce.
  //
  // It's important to note that the Abstract returned from decode isn't
  // required to be the same Abstract returned from the Dialect's interpret and
  // it is undefined behavior to treat them as the same C++ type. You'll first
  // need to negotiate the simulacra into the Abstract before you can use it as
  // the exact object.
  //
  // This nuance of simulacra is critical for optimal performance as it lets the
  // resulting Abstract to answer questions with the extra information that the
  // Abstract can keep a compact representation rather than materialize a 1:1
  // TTX graph of it's representation.
  virtual auto decode(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes encoding,
      Tetrodotoxin::Source::Abstract& context) -> Perimortem::Core::Option<Abstract&> {
    return {};
  }

  // By default Dialects don't produce any useful documentation so the override
  // is provided at this level to save on Dialect boiler plate.
  constexpr auto get_documentation() const
      -> const Tetrodotoxin::Source::Documentation& override {
    return Tetrodotoxin::Source::Documentation::get_empty();
  }
};

}  // namespace Tetrodotoxin::Language
