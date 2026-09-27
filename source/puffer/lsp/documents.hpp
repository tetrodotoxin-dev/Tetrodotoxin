// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "puffer/lsp/document.hpp"
#include "puffer/lsp/position_encoding.hpp"
#include "tetrodotoxin/app/dialect.hpp"
#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/repository/repository.hpp"
#include "tetrodotoxin/package/snapshots.hpp"
#include "tetrodotoxin/render/dialect.hpp"
#include "tetrodotoxin/scene/dialect.hpp"
#include "tetrodotoxin/shader/dialect.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/lexical/errors.hpp"
#include "tetrodotoxin/source/lexical/token.hpp"

namespace Puffer::Lsp {

// Documents keeps unsaved editor text with the Package session that interprets
// it. Replacing one file invalidates the shared session, while semantic work
// waits until an editor request needs a completed snapshot.
class Documents {
 public:
  class Diagnostics {
   public:
    constexpr Diagnostics(
        const Tetrodotoxin::Source::Lexical::Errors& errors,
        Perimortem::Core::View::Bytes source_name)
        : errors(errors), source_name(source_name) {}

    constexpr auto get_errors() const -> const Tetrodotoxin::Source::Lexical::Errors& {
      return errors;
    }

    constexpr auto get_source_name() const -> Perimortem::Core::View::Bytes {
      return source_name;
    }

   private:
    const Tetrodotoxin::Source::Lexical::Errors& errors;
    Perimortem::Core::View::Bytes source_name;
  };

  Documents(Tetrodotoxin::Package::Repository::Repository& repository);

  auto upsert(
      Perimortem::Core::View::Bytes uri,
      Perimortem::Core::View::Bytes source) -> void;
  auto erase(Perimortem::Core::View::Bytes uri) -> void;
  auto get_text(Perimortem::Core::View::Bytes uri) const
      -> Perimortem::Core::View::Bytes;
  auto find_semantic(
      Perimortem::Core::View::Bytes uri,
      const PositionEncoding::Position& position)
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&>;
  auto get_associations(Perimortem::Core::View::Bytes uri)
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Lexical::Associations&>;
  auto get_monograph(Perimortem::Core::View::Bytes uri)
      -> Perimortem::Core::Option<const Tetrodotoxin::Language::Monograph&>;
  auto get_completed_monograph(Perimortem::Core::View::Bytes uri)
      -> Perimortem::Core::Option<const Tetrodotoxin::Language::Monograph&>;
  auto get_tokens(Perimortem::Core::View::Bytes uri)
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Source::Lexical::Token>;
  auto find_definition(
      Perimortem::Core::View::Bytes source_uri,
      const Tetrodotoxin::Source::Abstract& semantic)
      -> Perimortem::Core::Option<
          Tetrodotoxin::Environment::Workspace::AuthoredLocation>;
  auto find_acquired_definition(
      Perimortem::Core::View::Bytes source_uri,
      const PositionEncoding::Position& position,
      const Tetrodotoxin::Source::Abstract& semantic)
      -> Perimortem::Core::Option<
          Tetrodotoxin::Environment::Workspace::AuthoredLocation>;
  auto resolve_uri(
      const Tetrodotoxin::Environment::Workspace::AuthoredLocation& authored)
      const -> Perimortem::Memory::Dynamic::Bytes;
  auto get_diagnostics(Perimortem::Core::View::Bytes uri)
      -> Perimortem::Core::Option<Diagnostics>;
  auto invalidate(Perimortem::Core::View::Bytes uri) -> void;
  auto set_position_encoding(PositionEncoding selected) -> void;
  auto get_position_encoding() const -> const PositionEncoding&;

 private:
  struct Session {
    Bool active = False;
    Perimortem::Memory::Dynamic::Bytes root;
    Perimortem::Core::Option<
        Perimortem::Memory::Dynamic::Record<Tetrodotoxin::Source::Lexical::Errors>>
        errors;
    Perimortem::Core::Option<Perimortem::Memory::Dynamic::Record<
        Tetrodotoxin::Environment::Workspace>>
        workspace;
  };

  auto find(Perimortem::Core::View::Bytes uri) const -> Count;
  auto find_session(Perimortem::Core::View::Bytes root) const -> Count;
  auto create_workspace(Document& document)
      -> Perimortem::Core::Option<Tetrodotoxin::Environment::Workspace&>;
  auto get_workspace(Document& document)
      -> Perimortem::Core::Option<Tetrodotoxin::Environment::Workspace&>;
  auto get_errors(Document& document)
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Lexical::Errors&>;
  auto invalidate_package(Perimortem::Core::View::Bytes root) -> void;
  auto select_session(Document& document) -> Perimortem::Core::Option<Session&>;

  // Editor sessions borrow these exact Dialects. Declaration order keeps each
  // dependency alive until the Toolchain and its Workspace sessions are gone.
  Tetrodotoxin::Library::Dialect library;
  Tetrodotoxin::Package::Dialect package;
  Tetrodotoxin::App::Dialect app;
  Tetrodotoxin::Render::Dialect render;
  Tetrodotoxin::Scene::Dialect scene;
  Tetrodotoxin::Shader::Dialect shader;
  Tetrodotoxin::Environment::Toolchain toolchain;
  Perimortem::Memory::Dynamic::Record<Tetrodotoxin::Package::Snapshots>
      snapshots;
  Perimortem::Core::Static::Vector<Document, 64> records;
  Perimortem::Core::Static::Vector<Session, 16> sessions;
  Tetrodotoxin::Package::Repository::Repository& repository;
  PositionEncoding position_encoding;
  Bool toolchain_ready = False;
};

}  // namespace Puffer::Lsp
