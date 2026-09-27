// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/import.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"
#include "tetrodotoxin/source/type.hpp"

namespace Tetrodotoxin::Language {

// A Monograph is the lasting semantic root created for one source transaction.
// It shares that Arena with source bytes, presentation facts, and every graph
// identity established by interpretation. Workspace retains the whole
// transaction whenever this root exists, including an incomplete edit that is
// useful to editor tooling.
//
// Linking settles edges after neighboring roots exist. Finalization admits the
// completed semantic island to Terminal production. A fixed child Monograph is
// appropriate only when the source directly authors meaning owned by that
// child language, as Shader does for its executable Library body.
class Monograph : public Tetrodotoxin::Source::Type {
 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Source::Type);

  virtual ~Monograph() = 0;

  Monograph(
      Perimortem::Memory::Allocator::Arena& domain,
      const Tetrodotoxin::Source::Abstract& language,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Abstract& context);

  Monograph(const Monograph&) = delete;
  Monograph(Monograph&&) = delete;
  auto operator=(const Monograph&) -> Monograph& = delete;
  auto operator=(Monograph&&) -> Monograph& = delete;

  TTX_DOCUMENTATION(documentation);

  constexpr auto get_language() const -> const Tetrodotoxin::Source::Abstract& {
    return language;
  }

  // Some Dialects build one fixed child language layer into their result. Exact
  // installed Dialect identity selects that child, which keeps the relationship
  // consistent with the Toolchain that interpreted the source.
  virtual auto get_layer(const Tetrodotoxin::Source::Abstract& requested) const
      -> Perimortem::Core::Option<const Monograph&>;

  // Environment acquires the exact source-local Import Aliases retained by
  // this Monograph. Imports do not enter the Type inventory they may resolve.
  virtual auto get_root() const -> const Tetrodotoxin::Source::Abstract&;

  virtual auto retain_import(
      const Import::Description& description,
      Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Associations&> associations = {})
      -> Bool;

  virtual constexpr auto get_imports() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<Import>> {
    return imports.get_view();
  }

  // Linking inside the source sees private and public imported Types. External
  // contextual queries continue through resolve_concept and observe only the
  // public surface.
  virtual auto resolve_lexical_context(Perimortem::Core::View::Bytes route)
      const -> const Tetrodotoxin::Source::Abstract&;

  // Linking begins after every source in the transaction has established stable
  // identities. Finalization follows as a second barrier where each Monograph
  // can validate edges that may cross into another source.
  virtual auto compose(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;
  virtual auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;
  virtual auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

  // Restored graphs cross the same Package barriers even though they have no
  // source Cursor. A persistent Dialect reports rejection through process
  // Diagnostics while these operations preserve the authored transaction order.
  virtual auto compose_restored() -> Bool;
  virtual auto link_restored() -> Bool;
  virtual auto finalize_restored() -> Bool;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

  auto get_layout() const -> const Tetrodotoxin::Source::Layout& override;

 protected:
  // Concrete source roots can compose their owned lookup surface with common
  // imports without falling through to the outer context. This keeps a fixed
  // child layer from recursing through its owning Monograph.
  auto resolve_type(Perimortem::Core::View::Bytes route, Visibility visibility)
      const -> const Tetrodotoxin::Source::Abstract&;

  // Keeping concrete facts in the Monograph's lifetime domain lets graph edges
  // remain valid for as long as Workspace exposes the source result.
  Perimortem::Memory::Allocator::Arena& domain;

  // Parsed Documentation shares the retained source Arena.
  const Tetrodotoxin::Source::Documentation& documentation;

  // The outer semantic context answers unresolved root queries. Borrowing it
  // keeps those bindings with their real owner while the Monograph retains only
  // its own source graph.
  Tetrodotoxin::Source::Abstract& context;

 private:
  const Tetrodotoxin::Source::Abstract& language;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Source::Reference<Import>> imports;
};

}  // namespace Tetrodotoxin::Language
