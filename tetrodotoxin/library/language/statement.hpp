// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/flow/scope.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"
#include "tetrodotoxin/source/documentation.hpp"

namespace Tetrodotoxin::Library::Language {

// Statement is one retained membership in source order, not another semantic
// identity. Its root is the complete outermost Pack returned for an expression
// Statement, or the exact declaration, control owner, or nested Block selected
// by grammar. That borrowed object remains the only Abstract in the graph. A
// compact operation table is fixed when grammar selects the root, avoiding both
// multiple inheritance and later concrete category inspection by Block.
//
// Leading Documentation belongs here because it describes participation in an
// executable sequence. The underlying semantic owner need not counterfeit a
// declaration merely to retain that presentation fact backed by source.
class Statement {
 private:
  class Continue {
   public:
    template <typename Owner>
    constexpr auto operator()(const Owner&) const -> Bool {
      return True;
    }
  };

  class NoBindingName {
   public:
    template <typename Owner>
    constexpr auto operator()(const Owner&) const
        -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
      return {};
    }
  };

  class NoBinding {
   public:
    template <typename Owner>
    constexpr auto operator()(const Owner&) const
        -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> {
      return {};
    }
  };

 public:
  template <
      typename Owner,
      typename Link,
      typename Finalize,
      typename ReachesNext = Continue,
      typename BindingName = NoBindingName,
      typename Binding = NoBinding>
  static constexpr auto create(
      Owner& root,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Link,
      Finalize,
      ReachesNext = {},
      BindingName = {},
      Binding = {}) -> Statement {
    static_assert(__is_base_of(Tetrodotoxin::Source::Abstract, Owner));
    return Statement(
        root, documentation, anchor,
        Operations{
          .link = [](Tetrodotoxin::Source::Abstract& root, Tetrodotoxin::Source::Lexical::Cursor& cursor,
                     Flow::Scope& scope) -> Bool {
            return Link{}(static_cast<Owner&>(root), cursor, scope);
          },
          .finalize = [](Tetrodotoxin::Source::Abstract& root,
                         Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void {
            Finalize{}(static_cast<Owner&>(root), cursor);
          },
          .reaches_next = [](const Tetrodotoxin::Source::Abstract& root) -> Bool {
            return ReachesNext{}(static_cast<const Owner&>(root));
          },
          .get_binding_name = [](const Tetrodotoxin::Source::Abstract& root)
              -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
            return BindingName{}(static_cast<const Owner&>(root));
          },
          .get_binding = [](const Tetrodotoxin::Source::Abstract& root)
              -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> {
            return Binding{}(static_cast<const Owner&>(root));
          },
        });
  }

  template <typename Link, typename Finalize>
  static constexpr auto create_pack(
      Model::Pack& pack,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Link,
      Finalize) -> Statement {
    return Statement(
        pack, documentation, anchor,
        PackOperations{
          .link = [](Model::Pack& pack, Tetrodotoxin::Source::Lexical::Cursor& cursor,
                     Flow::Scope& scope) -> Bool {
            return Link{}(pack, cursor, scope);
          },
          .finalize = [](Model::Pack& pack, Tetrodotoxin::Source::Lexical::Cursor& cursor)
              -> void { Finalize{}(pack, cursor); },
        });
  }

  constexpr auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor, Flow::Scope& scope)
      -> Bool {
    return pack != nullptr ? pack_operations.link(*pack, cursor, scope)
                           : operations.link(*root, cursor, scope);
  }

  constexpr auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> void {
    if (pack != nullptr) {
      pack_operations.finalize(*pack, cursor);
    } else {
      operations.finalize(*root, cursor);
    }
  }

  constexpr auto reaches_next() const -> Bool {
    return pack != nullptr ? True : operations.reaches_next(*root);
  }

  constexpr auto get_binding_name() const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
    return pack != nullptr
               ? Perimortem::Core::Option<Perimortem::Core::View::Bytes>()
               : operations.get_binding_name(*root);
  }

  constexpr auto get_binding() const
      -> Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> {
    return pack != nullptr
               ? Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&>()
               : operations.get_binding(*root);
  }

  auto get_root() const -> const Tetrodotoxin::Source::Abstract& {
    if (root != nullptr) {
      return *root;
    }
    auto identity = pack->get_identity();
    return identity ? *identity : Tetrodotoxin::Source::Unknown::get_unknown();
  }

  auto get_pack() const -> Perimortem::Core::Option<const Model::Pack&> {
    if (pack != nullptr) {
      return *pack;
    }
    return Model::Pack::from(static_cast<const Tetrodotoxin::Source::Abstract&>(*root));
  }

  constexpr auto get_documentation() const
      -> const Tetrodotoxin::Source::Documentation& {
    return *documentation;
  }

  constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor { return anchor; }

 private:
  struct Operations {
    Bool (*link)(Tetrodotoxin::Source::Abstract&, Tetrodotoxin::Source::Lexical::Cursor&, Flow::Scope&);
    void (*finalize)(Tetrodotoxin::Source::Abstract&, Tetrodotoxin::Source::Lexical::Cursor&);
    Bool (*reaches_next)(const Tetrodotoxin::Source::Abstract&);
    Perimortem::Core::Option<Perimortem::Core::View::Bytes> (*get_binding_name)(
        const Tetrodotoxin::Source::Abstract&);
    Perimortem::Core::Option<const Tetrodotoxin::Source::Abstract&> (*get_binding)(
        const Tetrodotoxin::Source::Abstract&);
  };

  struct PackOperations {
    Bool (*link)(Model::Pack&, Tetrodotoxin::Source::Lexical::Cursor&, Flow::Scope&);
    void (*finalize)(Model::Pack&, Tetrodotoxin::Source::Lexical::Cursor&);
  };

  constexpr Statement(
      Tetrodotoxin::Source::Abstract& root,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      Operations operations)
      : root(&root),
        documentation(&documentation),
        anchor(anchor),
        operations(operations) {}

  constexpr Statement(
      Model::Pack& pack,
      const Tetrodotoxin::Source::Documentation& documentation,
      Tetrodotoxin::Source::Lexical::Anchor anchor,
      PackOperations operations)
      : pack(&pack),
        documentation(&documentation),
        anchor(anchor),
        pack_operations(operations) {}

  Tetrodotoxin::Source::Abstract* root = nullptr;
  Model::Pack* pack = nullptr;
  const Tetrodotoxin::Source::Documentation* documentation;
  Tetrodotoxin::Source::Lexical::Anchor anchor;
  Operations operations{};
  PackOperations pack_operations{};
};

// Managed::Vector grows by relocating its entries as bytes. Keeping this
// assertion beside the record prevents a later ownership field from silently
// invalidating that storage contract.
static_assert(__is_trivially_copyable(Statement));

}  // namespace Tetrodotoxin::Library::Language
