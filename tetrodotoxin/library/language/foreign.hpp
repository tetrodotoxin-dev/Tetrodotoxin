// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/access/static.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/memory.hpp"
#include "tetrodotoxin/library/language/signature.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/source/declaration.hpp"
#include "tetrodotoxin/source/abstract.hpp"
#include "tetrodotoxin/source/reference.hpp"
#include "tetrodotoxin/source/unknown.hpp"
#include "tetrodotoxin/source/lexical/anchor.hpp"
#include "tetrodotoxin/source/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language {

// Foreign is the one external declaration context owned by a Library Source.
// It keeps block grammar, ABI agreement, lookup, and closure together while
// the parent Source remains the lexical context for declaration Type routes.
class Foreign final : public Tetrodotoxin::Source::Abstract {
 public:
  // State Types settle before Function signatures may consume them. Finalize is
  // a publication barrier only because bodyless declarations add no later graph
  // identities or evaluation work.
  enum class Stage : U8 {
    Authored,
    TypesLinked,
    CallablesLinked,
    Finalized,
  };

  // State is one external data declaration. Its Definition is hosted by the
  // Foreign context while its delayed Type route falls through to the Source.
  class State final : public Model::Memory {
   public:
    TTX_CONTRACT(State, Model::Memory);

    auto bind_interface(Perimortem::System::Uuid requested) const
        -> Perimortem::Utility::Result<
            Ttx::Semantic::Negotiation::Binding,
            Ttx::Semantic::Negotiation::Binding::Failure> override {
      if (requested == Tetrodotoxin::Source::Declaration::contract_id) {
        return Tetrodotoxin::Source::Declaration::provide(*this);
      }
      if (requested == Tetrodotoxin::Language::Definition::contract_id) {
        return Tetrodotoxin::Language::Definition::provide(*this);
      }
      if (requested == Tetrodotoxin::Source::Addressable::contract_id) {
        static const Tetrodotoxin::Source::Addressable::Operations operations = {
          [](const void* source) -> Ttx::Concept::Abstract {
            return static_cast<const State*>(source)
                ->type_reference.get_interface();
          },
        };
        return Ttx::Semantic::Negotiation::Binding::provide<Tetrodotoxin::Source::Addressable>(
            this, operations);
      }
      return Model::Memory::bind_interface(requested);
    }

    auto get_symbol() const -> Perimortem::Core::View::Bytes {
      return definition.get_name();
    }

    static auto create_authored(
        Perimortem::Memory::Allocator::Arena& domain,
        Tetrodotoxin::Language::Definition& definition,
        TypeReference type_reference,
        Perimortem::Core::View::Bytes abi) -> State&;

    static auto create(
        Perimortem::Memory::Allocator::Arena& domain,
        Tetrodotoxin::Language::Definition& definition,
        TypeReference type_reference,
        Perimortem::Core::View::Bytes abi) -> State&;

    State(const State&) = delete;
    State(State&&) = delete;
    auto operator=(const State&) -> State& = delete;
    auto operator=(State&&) -> State& = delete;

    auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

    auto link_restored_declaration_type() -> Bool;

    TTX_DOCUMENTATION(get_definition().get_documentation());
    TTX_NAME(definition.get_name());

    constexpr auto get_definition() const
        -> const Tetrodotoxin::Language::Definition& {
      return definition;
    }

    constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor {
      return definition.get_authored().get_anchor();
    }

    constexpr auto get_declaration_anchor() const
        -> Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> {
      const auto anchor = definition.get_authored().get_anchor();
      if (!anchor.get_span()) {
        return {};
      }

      return anchor;
    }

    auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;

    auto get_type() const -> const Tetrodotoxin::Source::Abstract& override;

    constexpr auto permits_write_from(const Model::Type&) const
        -> Bool override {
      return get_definition().get_visibility() ==
             Tetrodotoxin::Language::Visibility::Public;
    }

    constexpr auto get_type_reference() const -> const TypeReference& {
      return type_reference;
    }

    constexpr auto get_abi() const -> Perimortem::Core::View::Bytes {
      return abi;
    }

   private:
    friend class Tetrodotoxin::Source::Declaration;
    auto get_domain() const -> Ttx::Concept::Domain::Answer override {
      return type_reference.get_interface();
    }

    auto complete_source(
        Tetrodotoxin::Source::Declaration::Phase phase,
        Tetrodotoxin::Source::Lexical::Cursor* cursor)
        -> Tetrodotoxin::Source::Declaration::Completion {
      using Phase = Tetrodotoxin::Source::Declaration::Phase;
      if (phase == Phase::Type) {
        return link(*cursor);
      }
      if (phase == Phase::RestoredType) {
        return link_restored_declaration_type();
      }
      return True;
    }

    constexpr State(
        Tetrodotoxin::Language::Definition& definition,
        TypeReference type_reference,
        Perimortem::Core::View::Bytes abi)
        : definition(definition), type_reference(type_reference), abi(abi) {}

    Tetrodotoxin::Language::Definition& definition;
    TypeReference type_reference;
    Perimortem::Core::View::Bytes abi;
    Perimortem::Core::Option<Tetrodotoxin::Source::Reference<const Model::Type>> type;
  };

  // Function is one bodyless external Callable. Target owners consume the
  // retained ABI and symbol without making Foreign own native representation.
  class Function final : public Model::Callable {
   public:
    TTX_CONTRACT(Function, Model::Callable);

    auto bind_interface(Perimortem::System::Uuid requested) const
        -> Perimortem::Utility::Result<
            Ttx::Semantic::Negotiation::Binding,
            Ttx::Semantic::Negotiation::Binding::Failure> override {
      if (requested == Tetrodotoxin::Language::Definition::contract_id) {
        return Tetrodotoxin::Language::Definition::provide(*this);
      }
      return Model::Callable::bind_interface(requested);
    }

    static auto create_authored(
        Perimortem::Memory::Allocator::Arena& domain,
        Tetrodotoxin::Language::Definition& definition,
        Signature& signature,
        Perimortem::Core::View::Bytes abi) -> Function&;

    static auto create(
        Perimortem::Memory::Allocator::Arena& domain,
        Tetrodotoxin::Language::Definition& definition,
        Signature& signature,
        Perimortem::Core::View::Bytes abi) -> Function&;

    Function(const Function&) = delete;
    Function(Function&&) = delete;
    auto operator=(const Function&) -> Function& = delete;
    auto operator=(Function&&) -> Function& = delete;

    auto link(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

    auto link_restored_declaration_signature() -> Bool override;

    TTX_DOCUMENTATION(get_definition().get_documentation());
    TTX_NAME(definition.get_name());

    constexpr auto get_definition() const
        -> const Tetrodotoxin::Language::Definition& {
      return definition;
    }

    constexpr auto get_anchor() const -> Tetrodotoxin::Source::Lexical::Anchor {
      return definition.get_authored().get_anchor();
    }

    constexpr auto get_declaration_anchor() const
        -> Perimortem::Core::Option<Tetrodotoxin::Source::Lexical::Anchor> override {
      const auto anchor = definition.get_authored().get_anchor();
      if (!anchor.get_span()) {
        return {};
      }

      return anchor;
    }

    auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;

    auto resolve_concept(Perimortem::Core::View::Bytes) const
        -> const Tetrodotoxin::Source::Abstract& override;

    constexpr auto get_parameters() const
        -> const Tetrodotoxin::Source::Layout& override {
      return signature.get_parameters();
    }

    constexpr auto get_results() const -> const Tetrodotoxin::Source::Layout& override {
      return signature.get_results();
    }

    constexpr auto get_abi() const -> Perimortem::Core::View::Bytes {
      return abi;
    }

    constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
      return get_definition().get_name();
    }

    constexpr auto get_signature() const -> const Signature& {
      return signature;
    }

   private:
    constexpr Function(
        Tetrodotoxin::Language::Definition& definition,
        Signature& signature,
        Perimortem::Core::View::Bytes abi)
        : definition(definition), signature(signature), abi(abi) {}

    Tetrodotoxin::Language::Definition& definition;
    Signature& signature;
    Perimortem::Core::View::Bytes abi;
    Bool linked = False;
  };

  Foreign(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Source::Abstract& parent);

  Foreign(const Foreign&) = delete;
  Foreign(Foreign&&) = delete;
  auto operator=(const Foreign&) -> Foreign& = delete;
  auto operator=(Foreign&&) -> Foreign& = delete;

  TTX_CONTRACT(Foreign, Tetrodotoxin::Source::Abstract);
  TTX_NAME("foreign"_view);
  TTX_DOCUMENTATION(*documentation);

  // A closing Foreign block contributes one atomic set of declarations. The
  // parser validates repetition and ABI agreement before this model operation
  // changes the retained context.
  auto retain_block(
      const Tetrodotoxin::Source::Documentation& block_documentation,
      Perimortem::Core::View::Bytes selected_abi,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<State>> states,
      Perimortem::Core::View::Vector<Tetrodotoxin::Source::Reference<Function>>
          functions,
      Perimortem::Core::View::Vector<
          Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract>> declarations)
      -> Bool;

  auto link_types(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;
  auto link_callables(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;
  auto finalize(Tetrodotoxin::Source::Lexical::Cursor& cursor) -> Bool;

  auto link_restored() -> Bool;

  auto finalize_restored() -> Bool;

  constexpr auto is_authored() const -> Bool { return Bool(abi); }

  constexpr auto get_stage() const -> Stage { return stage; }

  constexpr auto get_abi() const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
    return abi;
  }

  constexpr auto get_states() const { return states.get_view(); }

  constexpr auto get_functions() const { return functions.get_view(); }

  constexpr auto get_declarations() const { return declarations.get_view(); }

  auto resolve() const -> const Tetrodotoxin::Source::Abstract& override;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Tetrodotoxin::Source::Abstract& override;

 private:
  auto retain_documentation(
      const Tetrodotoxin::Source::Documentation& block_documentation) -> void;

  Perimortem::Memory::Allocator::Arena& domain;
  Tetrodotoxin::Source::Abstract& parent;
  Access::Static& static_authority;
  const Tetrodotoxin::Source::Documentation* documentation;
  Perimortem::Core::Option<Perimortem::Core::View::Bytes> abi;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Source::Reference<State>> states;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Source::Reference<Function>>
      functions;
  Perimortem::Memory::Managed::Vector<
      Tetrodotoxin::Source::Reference<Tetrodotoxin::Source::Abstract>>
      declarations;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language
