// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/declarations/callable.h"
#include "ttx/data/form/representation.hpp"

namespace Ttx::Concept::Declarations {

// The callable question joins semantic arguments to their realized storage.
// Consumers prepare their own adapters from this description, then acquire
// executable state from the runtime instance. Keeping discovery separate lets
// an emitted factory outlive the entire graph that described its methods.
class Callable {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_CALLABLE_ID_HIGH,
    TTX_CALLABLE_ID_LOW,
  };
  using Api = ttx_callable;
  using Operations = ttx_callable_operations;

  explicit constexpr Callable(Api api) : api(api) {}

  // Argument order is a callable question, while byte order is a data question.
  // The ordered fields can therefore name placements in a padded or differently
  // arranged frame without changing the terminal's public argument order.
  class Frame {
   public:
    explicit constexpr Frame(ttx_callable_frame value) : value(value) {}

    auto get_representation() const -> const Data::Form::Representation& {
      return *value.representation;
    }

    auto get_size() const -> Count { return value.count; }
    auto get_subject(Count index) const -> Abstract {
      return Abstract(value.fields[index].subject);
    }

    auto get_offset(Count index) const -> Count {
      return value.fields[index].offset;
    }

   private:
    ttx_callable_frame value;
  };

  // Two methods may have identical frames and still perform different work.
  // The operation UUID carries that behavioral promise into runtime
  // acquisition, where the convention and frames establish its callable
  // realization.
  class Description {
   public:
    explicit constexpr Description(ttx_callable_description value)
        : value(value) {}

    auto get_contract() const -> Perimortem::System::Uuid {
      return Perimortem::System::Uuid(value.contract);
    }

    auto get_inputs() const -> Frame { return Frame(value.inputs); }
    auto get_outputs() const -> Frame { return Frame(value.outputs); }

   private:
    ttx_callable_description value;
  };

  // A bound description borrows the declaration. Calling describe can still
  // return Pending or Rejected, so an incomplete or restricted declaration
  // cannot accidentally become a publishable method.
  auto describe() const -> Perimortem::Utility::
      Result<Description, Semantic::Negotiation::Binding::Failure> {
    ttx_callable_description output;
    const auto status = api.operations->describe(api.source, &output);
    if (status == TTX_BINDING_SATISFIED) {
      return Description(output);
    }

    if (status == TTX_BINDING_UNSUPPORTED) {
      return Semantic::Negotiation::Binding::Failure::Unsupported;
    }

    if (status == TTX_BINDING_PENDING) {
      return Semantic::Negotiation::Binding::Failure::Pending;
    }

    return Semantic::Negotiation::Binding::Failure::Rejected;
  }

 private:
  Api api;
};

}  // namespace Ttx::Concept::Declarations

TTX_DATA_RECORD(
    ttx_callable_field,
    TTX_DATA_MEMBER(ttx_callable_field, subject),
    TTX_DATA_MEMBER(ttx_callable_field, offset));

TTX_DATA_RECORD(
    ttx_callable_frame,
    TTX_DATA_MEMBER(ttx_callable_frame, representation),
    TTX_DATA_MEMBER(ttx_callable_frame, fields),
    TTX_DATA_MEMBER(ttx_callable_frame, count));

TTX_DATA_RECORD(
    ttx_callable_description,
    TTX_DATA_MEMBER(ttx_callable_description, contract),
    TTX_DATA_MEMBER(ttx_callable_description, inputs),
    TTX_DATA_MEMBER(ttx_callable_description, outputs));

TTX_DATA_RECORD(
    ttx_callable_operations,
    TTX_DATA_MEMBER(ttx_callable_operations, describe));

TTX_DATA_RECORD(
    ttx_callable,
    TTX_DATA_MEMBER(ttx_callable, source),
    TTX_DATA_MEMBER(ttx_callable, operations));
