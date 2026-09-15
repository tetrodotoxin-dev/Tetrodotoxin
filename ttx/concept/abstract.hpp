// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/system/uuid.hpp"

#include "ttx/concept/abstract.h"
#include "ttx/concept/answers/none.h"
#include "ttx/concept/visitor.hpp"
#include "ttx/semantic/negotiation/query.hpp"

namespace Ttx::Concept {

// An Abstract is the view of a subject that its provider agreed to expose.
// Keeping only the C receiver and table makes that same view usable whether
// the subject is a native object, a device resource or a foreign publication.
// Navigation and binding therefore use the same interface across those
// implementations. C++ can add local shortcuts while keeping that common
// contract available to substitutes.
//
// The table's complete callable form participates in TTX's semantic binding.
// Once this record is acquired, navigation can use those Abstract operations
// directly. The chosen two pointer carrier keeps graph edges compact without
// making that carrier a requirement for other contracts.
//
// This value borrows the subject and executable table for the lifetime promised
// by its publication. Binding selects a capability of the encountered policy.
// It does not resolve past that policy or grant access to its private storage.
class Abstract {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_ABSTRACT_ID_HIGH,
    TTX_ABSTRACT_ID_LOW,
  };
  using Api = ttx_abstract;
  using Operations = ttx_abstract_ops;
  using Visitor = Concept::Visitor<Abstract>;

  static auto accept(Api api) -> Bool { return api.source && api.operations; }

  constexpr Abstract(const void* source, const Operations& operations)
      : value{source, &operations} {}
  explicit constexpr Abstract(Api value) : value(value) {}

  constexpr auto get_abi() const -> Api { return value; }
  constexpr auto get_query() const -> Semantic::Negotiation::Query {
    return Semantic::Negotiation::Query({value.source, value.operations->bind});
  }

  auto bind_interface(
      Perimortem::System::Uuid contract,
      Data::Form::Storage destination) const
      -> Semantic::Negotiation::Binding::Status {
    return get_query().bind(contract, destination);
  }

  template <typename Contract>
  auto bind() const -> Perimortem::Utility::
      Result<Contract, Semantic::Negotiation::Binding::Failure> {
    if constexpr (__is_same(Contract, Abstract)) {
      return *this;
    } else {
      return get_query().template bind<Contract>();
    }
  }

  // Gets the data of this Abstract. If a canonical form is required then first
  // call `resolve()` to peel away any domain layers to get to the base object.
  //
  // `get_data` is a useful `resolve_concept` side channel for quickly passing
  // bytes between two systems. The interface provides two unique promises that
  // can't be covered by resolve concept that make it a useful extension:
  //
  // * The bytes it returns have no Abstract contract to be inspected.
  // * The data can never be assumed as `Constant` under any observation.
  //
  // This makes get_data useful for sockets or streams returning large amounts
  // of structured data. All Abstract calls are synchronous for the caller, so
  // waiting for data here also blocks that caller.
  // The returned view survives until the next observation on this subject or
  // publication release. A stronger retention promise belongs to the supplying
  // contract. Consumers can copy the bytes when they need an independent
  // lifetime.
  auto get_data() const -> Perimortem::Core::View::Bytes {
    const auto data = value.operations->get_data(value.source);
    return Perimortem::Core::View::Bytes(data.data, data.size);
  }

  // Returns the Abstract represented by this observation by removing any domain
  // specific layers. Resolve does not define exactly what that representation
  // is and is only defined as being idempotent for an unchanged observation
  // state meaning: `abstract.resolve() == abstract.resolve().resolve()`.
  // Equality here identifies the same borrowed view, not the C++ value that
  // carries it or a claim about equivalent views from different publications.
  auto resolve() const -> Abstract {
    return Abstract(value.operations->resolve(value.source));
  }

  // Resolves one binary concept owned by this Abstract. The route format is an
  // arbitrary byte query that is forwarded to the abstract. If the route is not
  // `Constant` then resolving the same query is a separate observation and may
  // return a different value.
  //
  // Names can be useful when establishing meta routes and queries, but the
  // bytes can also encode instructions for negotiation between domains.
  auto resolve_concept(Perimortem::Core::View::Bytes route) const -> Abstract {
    return Abstract(value.operations->resolve_concept(
        value.source, {route.get_data(), route.get_size()}));
  }

  // The owner advertises the routes visible at this boundary. It can walk its
  // own representation directly instead of building a Pack for the caller to
  // unpack, trading Pack construction for callback overhead. Routes belong to
  // this scope, so enumeration order carries no meaning and must not be used to
  // infer implementation details. Like `resolve_concept`, visitation is an
  // observation transaction and can return different values on each visitation.
  //
  // Calls are synchronous and the receiver must not invalidate the provider's
  // traversed state. A route only survives its callback, so consumers must copy
  // it if they need to retain it.
  // This means encoding pointers in the route should point to stable locations
  // and should mostly be avoided. Abstracts keep their normal graph lifetime.
  auto visit_concepts(Visitor visitor) const -> void {
    const ttx_concept_visitor receiver = {
      &visitor, [](void* source, perimortem_view_bytes route, Api subject) {
        (*static_cast<Visitor*>(source))(
            Perimortem::Core::View::Bytes(route.data, route.size),
            Abstract(subject));
      }};
    value.operations->visit_concepts(value.source, receiver);
  }

  // Checks to see if this Abstract satisfies another projection based on the
  // projection that can be derived from requirement. This allows for higher
  // order queries outside of the TTX contract system, but nothing prevents
  // systems from implementing satisfies using contracts.
  auto satisfies(const Abstract& requirement) const -> Bool {
    return value.operations->satisfies(value.source, requirement.get_abi()) !=
           0;
  }

  // This token identifies the provider within its enclosing publication. It
  // proves neither a C++ type nor identity across independent publications.
  constexpr auto get_identity() const -> const void* { return value.source; }

  constexpr auto operator==(const Abstract& other) const -> Bool {
    return value.source == other.value.source &&
           value.operations == other.value.operations;
  }
  constexpr auto operator!=(const Abstract& other) const -> Bool {
    return !(*this == other);
  }

  // A cast is available only when this module published this exact C++ owner
  // type. Its private table proves both the state type and the pointer
  // adjustment made at publication. A binding from another module declines the
  // shortcut even if the cast would happen to work, since this module cannot
  // establish that other module's C++ type identity.
  //
  // The portable receiver is const. Making its carrier mutable cannot grant
  // permission to mutate its owner, so this shortcut returns only a const
  // borrow.
  template <typename Owner>
  auto cast() const -> Perimortem::Core::Option<const Owner&> {
    if (value.operations != &provider_operations<Owner>()) {
      return {};
    }

    return *static_cast<const Owner*>(value.source);
  }

  // The provider keeps its own storage and supplies the same operations as a
  // foreign publication. Generated thunks recover only that known owner type.
  // Optional operations use leaf defaults without requiring another graph or
  // allocating an adapter for each subject.
  template <typename Owner>
  static constexpr auto provide(const Owner& owner) -> Abstract {
    if constexpr (
        __is_base_of(Abstract, Owner) &&
        __is_same(decltype(&Owner::get_data), decltype(&Abstract::get_data))) {
      return static_cast<const Abstract&>(owner);
    } else {
      return Abstract(&owner, provider_operations<Owner>());
    }
  }

 private:
  // Hidden linkage keeps table identity local to one linked image. Otherwise
  // symbol preemption could accidentally turn a foreign publication into a
  // native cast proof when two modules instantiate the same C++ provider type.
  template <typename Owner>
  __attribute__((visibility("hidden"))) static constexpr auto
      provider_operations() -> const Operations& {
    static constexpr Operations operations = {
      [](const void* source, perimortem_uuid id,
         ttx_storage requested) -> ttx_binding_status {
        const Data::Form::Storage target(requested);
        if (Perimortem::System::Uuid(id) == contract_id) {
          return static_cast<ttx_binding_status>(
              Semantic::Negotiation::Binding::provide<Abstract>(
                  Api(source, &operations), target));
        }

        if constexpr (requires { &Owner::bind_interface; }) {
          if constexpr (!__is_same(
                            decltype(&Owner::bind_interface),
                            decltype(&Abstract::bind_interface))) {
            return static_cast<ttx_binding_status>(
                static_cast<const Owner*>(source)->bind_interface(
                    Perimortem::System::Uuid(id), target));
          }
        }
        return TTX_BINDING_UNSUPPORTED;
      },
      [](const void* source) -> perimortem_view_bytes {
        const auto data = static_cast<const Owner*>(source)->get_data();
        return {data.get_data(), data.get_size()};
      },
      [](const void* source) -> Api {
        if constexpr (requires { &Owner::resolve; }) {
          if constexpr (!__is_same(
                            decltype(&Owner::resolve),
                            decltype(&Abstract::resolve))) {
            return static_cast<const Owner*>(source)->resolve().get_abi();
          }
        }
        return {source, &operations};
      },
      [](const void* source, perimortem_view_bytes route) -> Api {
        if constexpr (requires { &Owner::resolve_concept; }) {
          if constexpr (!__is_same(
                            decltype(&Owner::resolve_concept),
                            decltype(&Abstract::resolve_concept))) {
            return static_cast<const Owner*>(source)
                ->resolve_concept(
                    Perimortem::Core::View::Bytes(route.data, route.size))
                .get_abi();
          }
        }
        return ttx_none();
      },
      [](const void* source, ttx_concept_visitor visitor) {
        if constexpr (requires { &Owner::visit_concepts; }) {
          if constexpr (!__is_same(
                            decltype(&Owner::visit_concepts),
                            decltype(&Abstract::visit_concepts))) {
            auto receive = [&](Perimortem::Core::View::Bytes route,
                               Abstract subject) {
              visitor.receive(
                  visitor.source, {route.get_data(), route.get_size()},
                  subject.get_abi());
            };
            static_cast<const Owner*>(source)->visit_concepts(Visitor(receive));
          }
        }
      },
      [](const void* source, Api requirement) -> U8 {
        if constexpr (requires { &Owner::satisfies; }) {
          if constexpr (!__is_same(
                            decltype(&Owner::satisfies),
                            decltype(&Abstract::satisfies))) {
            return bool(static_cast<const Owner*>(source)->satisfies(
                Abstract(requirement)));
          }
        }
        return 0;
      }};
    return operations;
  }

  Api value;
};

static_assert(sizeof(Abstract) == sizeof(ttx_abstract));

}  // namespace Ttx::Concept

TTX_DATA_RECORD(
    perimortem_view_bytes,
    TTX_DATA_MEMBER(perimortem_view_bytes, data),
    TTX_DATA_MEMBER(perimortem_view_bytes, size));

// Abstract navigation returns Abstracts and passes them to its visitor. These
// are recursive native declarations, so their Schema nodes share one constant
// owner instead of expanding the recursion through C++ template instantiation.
template <>
class Ttx::Data::Form::Native<ttx_abstract> {
  struct Definition {
    Schema root;
    Schema operations;
    Schema visitor;
    Schema receive;
    Schema resolve;
    Schema lookup;
    Schema visit;
    Schema satisfies;
    Schema::Argument receive_arguments[3];
    Schema::Argument resolve_arguments[1];
    Schema::Argument lookup_arguments[2];
    Schema::Argument visit_arguments[2];
    Schema::Argument satisfies_arguments[2];
    Schema::Position root_fields[2];
    Schema::Position operation_fields[6];
    Schema::Position visitor_fields[2];

    constexpr Definition()
        : root(
              Schema::composite(
                  {root_fields, 2},
                  sizeof(ttx_abstract),
                  alignof(ttx_abstract))),
          operations(
              Schema::composite(
                  {operation_fields, 6},
                  sizeof(ttx_abstract_ops),
                  alignof(ttx_abstract_ops))),
          visitor(
              Schema::composite(
                  {visitor_fields, 2},
                  sizeof(ttx_concept_visitor),
                  alignof(ttx_concept_visitor))),
          receive(
              Schema::callable(
                  Schema::Abi::SystemVAMD64,
                  {receive_arguments, 3})),
          resolve(
              Schema::callable(
                  Schema::Abi::SystemVAMD64,
                  {resolve_arguments, 1},
                  root)),
          lookup(
              Schema::callable(
                  Schema::Abi::SystemVAMD64,
                  {lookup_arguments, 2},
                  root)),
          visit(
              Schema::callable(
                  Schema::Abi::SystemVAMD64,
                  {visit_arguments, 2})),
          satisfies(
              Schema::callable(
                  Schema::Abi::SystemVAMD64,
                  {satisfies_arguments, 2},
                  Native<U8>::reference)),
          receive_arguments{
            Schema::pointer(), Native<perimortem_view_bytes>::reference,
            Schema::Argument(root)},
          resolve_arguments{Schema::pointer()},
          lookup_arguments{
            Schema::pointer(), Native<perimortem_view_bytes>::reference},
          visit_arguments{Schema::pointer(), Schema::Argument(visitor)},
          satisfies_arguments{Schema::pointer(), Schema::Argument(root)},
          root_fields{
            {Schema::pointer(), offsetof(ttx_abstract, source)},
            {Schema::pointer(&operations), offsetof(ttx_abstract, operations)}},
          operation_fields{
            TTX_DATA_MEMBER(ttx_abstract_ops, bind),
            TTX_DATA_MEMBER(ttx_abstract_ops, get_data),
            {resolve, offsetof(ttx_abstract_ops, resolve)},
            {lookup, offsetof(ttx_abstract_ops, resolve_concept)},
            {visit, offsetof(ttx_abstract_ops, visit_concepts)},
            {satisfies, offsetof(ttx_abstract_ops, satisfies)}},
          visitor_fields{
            {Schema::pointer(), offsetof(ttx_concept_visitor, source)},
            {receive, offsetof(ttx_concept_visitor, receive)}} {}
  };

  static const Definition definition;

 public:
  static constexpr const Schema& schema = definition.root;
  static constexpr Schema::Reference reference = schema;
};

inline constexpr Ttx::Data::Form::Native<ttx_abstract>::Definition
    Ttx::Data::Form::Native<ttx_abstract>::definition;

TTX_DATA_RECORD(
    ttx_concept_visitor,
    TTX_DATA_MEMBER(ttx_concept_visitor, source),
    TTX_DATA_MEMBER(ttx_concept_visitor, receive));
