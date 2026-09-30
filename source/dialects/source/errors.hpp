// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/dialects/source/anchor.hpp"
#include "tetrodotoxin/dialects/source/capabilities/diagnostics.hpp"
#include "tetrodotoxin/dialects/source/cursor.hpp"

namespace Tetrodotoxin::Dialects::Source {

// One Errors value is the publication boundary for diagnostics tied to authored
// text. Rendering happens after parsing, so the first report for an exact
// source name retains both the name and body needed to interpret every later
// Anchor. Reusing that snapshot prevents a repeated name from silently
// changing the text beneath an earlier diagnostic. One collection therefore
// represents one snapshot per path. Successive versions use separate Errors.
class Errors {
 public:
  // Report accumulates a complete message before publishing it at scope exit.
  // This keeps partially streamed messages out of Errors and gives every early
  // return the same publication behavior. Every context argument is explicit
  // because an absent body or range must be a deliberate choice by an owner
  // that actually has textual context.
  class Report {
   public:
    Report(
        Errors& errors,
        Perimortem::Core::View::Bytes source_name,
        Perimortem::Core::View::Bytes source_text,
        Anchor anchor);
    ~Report();
    Report(const Report&) = delete;
    Report(Report&&) = delete;
    auto operator=(const Report&) -> Report& = delete;
    auto operator=(Report&&) -> Report& = delete;

    template <typename value_type>
    auto operator<<(const value_type& value) -> Report& {
      message << value;
      return *this;
    }

    auto get_hint() -> Perimortem::Serialization::Stream::Textual<
        Perimortem::Memory::Managed::Bytes>& {
      return hint;
    }

   private:
    Errors& errors;
    Perimortem::Core::View::Bytes source_name;
    Perimortem::Core::View::Bytes source_text;
    Anchor anchor;
    Perimortem::Memory::Managed::Bytes message_storage;
    Perimortem::Memory::Managed::Bytes hint_storage;
    Perimortem::Serialization::Stream::Textual<
        Perimortem::Memory::Managed::Bytes>
        message;
    Perimortem::Serialization::Stream::Textual<
        Perimortem::Memory::Managed::Bytes>
        hint;
  };

  Errors() : source_map(arena), errors(arena) {}
  Errors(const Errors&) = delete;
  Errors(Errors&&) = delete;

  // Grammar code usually knows the friendlier message, while Errors can always
  // describe the exact Token mismatch. A supplied message becomes the main
  // diagnostic and keeps that lexical comparison as a useful hint.
  auto require(
      Cursor& cursor,
      Code::Type type,
      Perimortem::Core::View::Bytes message = {}) -> Source::Token {
    if (!cursor.matches(type)) {
      Code code(type);
      Perimortem::Core::Static::Bytes<128> hint_buffer;
      Perimortem::Core::Writer::Textual hint_message(hint_buffer);
      hint_message << "Expected lexical token "_view << code.get_semantics()
                   << " but got "_view
                   << cursor.current().get_code().get_semantics() << "."_view;

      // With no richer message, the lexical comparison is the useful error.
      // Otherwise the grammar message leads and the comparison adds context.
      if (message.is_empty()) {
        create_token_error(cursor, hint_message);
      } else {
        create_token_error(cursor, message, hint_message);
      }

      return Source::Token();
    }

    return cursor.consume();
  }

  // Errors copies message text into its Arena. Callers can safely assemble that
  // text in temporary buffers and choose the source focus that best explains
  // the failure.
  auto create_error(
      const Cursor& cursor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void {
    create_expression_error(cursor, Anchor::create(Span()), message, hint);
  }

  auto create_token_error(
      const Cursor& cursor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void {
    create_expression_error(
        cursor, Anchor::create(Span(cursor.current())), message, hint);
  }

  auto create_token_error(
      const Cursor& cursor,
      Source::Token token,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void {
    create_expression_error(cursor, Anchor::create(Span(token)), message, hint);
  }

  // A Span uses its opening Token as the natural focus. Semantic owners that
  // know a more useful Token can pass an Anchor while keeping the broader range
  // for the editor.
  auto create_expression_error(
      const Cursor& cursor,
      Source::Span span,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void {
    create_expression_error(cursor, Anchor::create(span), message, hint);
  }

  auto create_expression_error(
      const Cursor& cursor,
      Perimortem::Core::Option<Source::Anchor> anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void {
    if (!anchor) {
      create_error(cursor, message, hint);
      return;
    }
    create_expression_error(cursor, *anchor, message, hint);
  }

  auto create_expression_error(
      const Cursor& cursor,
      Source::Anchor anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void {
    Errors::Report report(
        *this, cursor.get_source_path(), cursor.get_source_text(), anchor);
    report << message;
    report.get_hint() << hint;
  }

  // A structured Report uses the supplied cursor only to identify its source.
  // The collector owns the message and retained evidence.
  auto create_report(const Cursor& cursor, Source::Span span)
      -> Errors::Report {
    return create_report(cursor, Anchor::create(span));
  }

  auto create_report(
      const Cursor& cursor,
      Perimortem::Core::Option<Source::Anchor> anchor) -> Errors::Report {
    return create_report(
        cursor, anchor ? *anchor : Anchor::create(Source::Span()));
  }

  auto create_report(const Cursor& cursor, Source::Anchor anchor)
      -> Errors::Report {
    return Errors::Report(
        *this, cursor.get_source_path(), cursor.get_source_text(), anchor);
  }

  auto get_diagnostics() -> Capabilities::Diagnostics {
    return Capabilities::Diagnostics::provide(*this);
  }
  auto report(
      tetrodotoxin_source_snapshot input,
      Anchor anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint) -> void;
  auto get_data() const -> Perimortem::Core::View::Bytes { return {}; }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage target) const
      -> Ttx::Semantic::Negotiation::Binding::Status;

  // Rendering remains delayed so callers can collect failures across one
  // transaction before choosing how to present them. The caller Arena owns only
  // the rendered view while the retained source snapshot stays canonical here.
  // A presentation owner may supply a physical display path without changing
  // the source key used by Workspace and editor tooling.
  auto render_message(
      Perimortem::Memory::Allocator::Arena& arena,
      Count index,
      Perimortem::Core::View::Bytes display_source_name = {}) const
      -> Perimortem::Core::View::Bytes;

  constexpr auto get_message(Count index) const
      -> Perimortem::Core::View::Bytes {
    return index < errors.get_size() ? errors.at(index).message
                                     : Perimortem::Core::View::Bytes();
  }

  constexpr auto get_anchor(Count index) const -> Anchor {
    return index < errors.get_size() ? errors.at(index).anchor
                                     : Anchor::create(Span());
  }

  constexpr auto get_source_name(Count index) const
      -> Perimortem::Core::View::Bytes {
    return index < errors.get_size() ? errors.at(index).source_name
                                     : Perimortem::Core::View::Bytes();
  }

  constexpr auto is_empty() const -> Bool { return get_size() == 0; }
  constexpr auto get_size() const -> Count { return errors.get_size(); }

 private:
  // Error stores Anchors against the canonical source snapshot rather than
  // retaining rendered lines that would duplicate the source for every report.
  struct Error {
    Perimortem::Core::View::Bytes message;
    Perimortem::Core::View::Bytes hint;
    Perimortem::Core::View::Bytes source_name;
    Anchor anchor;
  };

  auto retain_source(
      Perimortem::Core::View::Bytes source_name,
      Perimortem::Core::View::Bytes source_text)
      -> Perimortem::Core::View::Bytes;

  auto publish_report(
      Perimortem::Core::View::Bytes source_name,
      Perimortem::Core::View::Bytes source_text,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint,
      Anchor anchor) -> void;

  // Diagnostics are already the failure path, so one Arena favors stable views
  // and bulk release over reclaiming each message independently. The source map
  // also prevents repeated reports from paying for repeated source bodies.
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Perimortem::Core::View::Bytes>
          source_map;
  Perimortem::Memory::Managed::Vector<Error> errors;
};

}  // namespace Tetrodotoxin::Dialects::Source
